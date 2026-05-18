//  Program:      nes-py
//  File:         emulator.cpp
//  Description:  This class houses the logic and data for an NES emulator
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "nes_emu/emulator.hpp"
#include "nes_emu/mapper_factory.hpp"
#include "nes_emu/log.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace NES {

Emulator::Snapshot::Snapshot() :
    bus(),
    picture_bus(),
    cpu(),
    ppu(),
    mapper(nullptr) { }

Emulator::Snapshot::Snapshot(const Snapshot& other) :
    bus(other.bus),
    picture_bus(other.picture_bus),
    cpu(other.cpu),
    ppu(other.ppu),
    mapper(other.mapper == nullptr ? nullptr : other.mapper->clone()) { }

Emulator::Snapshot& Emulator::Snapshot::operator=(const Snapshot& other) {
    if (this != &other) {
        bus = other.bus;
        picture_bus = other.picture_bus;
        cpu = other.cpu;
        ppu = other.ppu;
        mapper = other.mapper == nullptr ? nullptr : other.mapper->clone();
    }
    return *this;
}

Emulator::Emulator(std::string rom_path) :
    mapper_observes_cpu_cycles(false) {
    // Wire fixed devices once so CPU bus I/O avoids hash/callback dispatch.
    bus.connect_devices(&cpu, &ppu, &picture_bus, controllers);
    // set the interrupt callback for the PPU
    ppu.set_interrupt_callback([&]() { cpu.interrupt(bus, CPU::NMI_INTERRUPT); });
    // load the ROM from disk, expect that the Python code has validated it
    cartridge.loadFromFile(rom_path);
    // create the mapper based on the mapper ID in the iNES header of the ROM
    mapper = MapperFactory(&cartridge);
    if (mapper == nullptr)
        throw std::runtime_error("unsupported mapper");
    wire_mapper();
}

void Emulator::wire_mapper() {
    mapper->setIRQCallback([&]() {
        cpu.interrupt(bus, CPU::IRQ_INTERRUPT);
    });
    mapper_observes_cpu_cycles = mapper->observesCPUCycles();
    bus.set_mapper(mapper.get());
    picture_bus.set_mapper(mapper.get());
}

void Emulator::synchronize_mapper_mirroring() {
    if (mapper != nullptr && mapper->hasNameTableMirroringChanged())
        picture_bus.update_mirroring();
}

void Emulator::advance_ppu_timing_cycle() {
    // 3 PPU steps per CPU step
    ppu.cycle(picture_bus);
    ppu.cycle(picture_bus);
    ppu.cycle(picture_bus);
    if (mapper_observes_cpu_cycles)
        mapper->onCPUCycle();
}

void Emulator::step_cycle_by_cycle() {
    for (int i = 0; i < CYCLES_PER_FRAME; i++) {
        advance_ppu_timing_cycle();
        cpu.cycle(bus);
        synchronize_mapper_mirroring();
    }
}

void Emulator::step_instruction_batched() {
    for (int i = 0; i < CYCLES_PER_FRAME; ) {
        advance_ppu_timing_cycle();

        if (!cpu.can_execute_instruction()) {
            cpu.cycle(bus);
            synchronize_mapper_mirroring();
            ++i;
            continue;
        }

        int instruction_cycles = cpu.execute_instruction(bus);
        synchronize_mapper_mirroring();
        ++i;

        const int remaining_frame_cycles = CYCLES_PER_FRAME - i;
        const int batched_cycles = std::min(
            instruction_cycles - 1,
            remaining_frame_cycles
        );
        for (int cycle = 0; cycle < batched_cycles; ++cycle)
            advance_ppu_timing_cycle();
        cpu.consume_pending_cycles(batched_cycles);
        i += batched_cycles;
    }
}

void Emulator::step() {
    if (can_batch_cpu_instructions())
        step_instruction_batched();
    else
        step_cycle_by_cycle();
}

void Emulator::backup() {
    Snapshot snapshot = save_state();
    backup_mapper = std::move(snapshot.mapper);
    backup_bus = snapshot.bus;
    backup_picture_bus = snapshot.picture_bus;
    backup_cpu = snapshot.cpu;
    backup_ppu = snapshot.ppu;
}

void Emulator::restore() {
    if (backup_mapper == nullptr)
        return;

    Snapshot snapshot;
    snapshot.mapper = backup_mapper->clone();
    snapshot.bus = backup_bus;
    snapshot.picture_bus = backup_picture_bus;
    snapshot.cpu = backup_cpu;
    snapshot.ppu = backup_ppu;
    load_state(snapshot);
}

Emulator::Snapshot Emulator::save_state() const {
    Snapshot snapshot;
    if (mapper == nullptr)
        throw std::runtime_error("cannot snapshot emulator without a mapper");
    snapshot.mapper = mapper->clone();
    snapshot.bus = bus.save_state();
    snapshot.picture_bus = picture_bus.save_state();
    snapshot.cpu = cpu;
    snapshot.ppu = ppu.save_state();
    return snapshot;
}

Emulator::Snapshot* Emulator::create_snapshot() const {
    Snapshot snapshot = save_state();
    return new Snapshot(std::move(snapshot));
}

void Emulator::load_state(const Snapshot& snapshot) {
    if (snapshot.mapper == nullptr)
        throw std::runtime_error("cannot restore an empty emulator snapshot");

    mapper = snapshot.mapper->clone();
    bus.load_state(snapshot.bus);
    picture_bus.load_state(snapshot.picture_bus);
    cpu = snapshot.cpu;
    ppu.load_state(snapshot.ppu);
    wire_mapper();
    synchronize_mapper_mirroring();
}

void Emulator::load_snapshot(const Snapshot* snapshot) {
    if (snapshot == nullptr)
        throw std::runtime_error("cannot restore a null emulator snapshot");
    load_state(*snapshot);
}

}  // namespace NES
