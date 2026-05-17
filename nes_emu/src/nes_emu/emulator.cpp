//  Program:      nes-py
//  File:         emulator.cpp
//  Description:  This class houses the logic and data for an NES emulator
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "nes_emu/emulator.hpp"
#include "nes_emu/mapper_factory.hpp"
#include "nes_emu/log.hpp"
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

void Emulator::step() {
    // render a single frame on the emulator
    for (int i = 0; i < CYCLES_PER_FRAME; i++) {
        // 3 PPU steps per CPU step
        ppu.cycle(picture_bus);
        ppu.cycle(picture_bus);
        ppu.cycle(picture_bus);
        if (mapper_observes_cpu_cycles)
            mapper->onCPUCycle();
        cpu.cycle(bus);
        synchronize_mapper_mirroring();
    }
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
