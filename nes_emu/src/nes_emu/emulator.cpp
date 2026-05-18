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

namespace NES {

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
    backup_mapper = mapper->clone();
    backup_bus = bus.save_state();
    backup_picture_bus = picture_bus.save_state();
    backup_cpu = cpu;
    backup_ppu = ppu.save_state();
}

void Emulator::restore() {
    if (backup_mapper == nullptr)
        return;

    mapper = backup_mapper->clone();
    bus.load_state(backup_bus);
    picture_bus.load_state(backup_picture_bus);
    cpu = backup_cpu;
    ppu.load_state(backup_ppu);
    wire_mapper();
}

}  // namespace NES
