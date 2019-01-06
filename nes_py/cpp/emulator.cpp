//  Program:      nes-py
//  File:         emulator.cpp
//  Description:  This class houses the logic and data for an NES emulator
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "emulator.hpp"
#include "log.hpp"

Emulator::Emulator(std::string path) : rom_path(path), cpu(), ppu() {
    // raise an error if IO callback setup fails
    if (
        !bus.set_read_callback(PPUSTATUS, [&](void) {return ppu.getStatus();}) ||
        !bus.set_read_callback(PPUDATA, [&](void) {return ppu.getData(picture_bus);}) ||
        !bus.set_read_callback(JOY1, [&](void) {return controller1.read();}) ||
        !bus.set_read_callback(JOY2, [&](void) {return controller2.read();}) ||
        !bus.set_read_callback(OAMDATA, [&](void) {return ppu.getOAMData();})
    ) {
        LOG(Error) << "Critical error: Failed to set I/O callbacks" << std::endl;
    }
    // raise an error if IO callback setup fails
    if (
        !bus.set_write_callback(PPUCTRL, [&](uint8_t b) {ppu.control(b);}) ||
        !bus.set_write_callback(PPUMASK, [&](uint8_t b) {ppu.setMask(b);}) ||
        !bus.set_write_callback(OAMADDR, [&](uint8_t b) {ppu.setOAMAddress(b);}) ||
        !bus.set_write_callback(PPUADDR, [&](uint8_t b) {ppu.setDataAddress(b);}) ||
        !bus.set_write_callback(PPUSCROL, [&](uint8_t b) {ppu.setScroll(b);}) ||
        !bus.set_write_callback(PPUDATA, [&](uint8_t b) {ppu.setData(picture_bus, b);}) ||
        !bus.set_write_callback(OAMDMA, [&](uint8_t b) {DMA(b);}) ||
        !bus.set_write_callback(JOY1, [&](uint8_t b) {controller1.strobe(b); controller2.strobe(b);}) ||
        !bus.set_write_callback(OAMDATA, [&](uint8_t b) {ppu.setOAMData(b);})
    ) {
        LOG(Error) << "Critical error: Failed to set I/O callbacks" << std::endl;
    }
    // set the interrupt callback for the PPU
    ppu.setInterruptCallback([&](){ cpu.interrupt(bus, CPU::NMI); });
    // load the ROM from disk and return if the operation fails
    if (!cartridge.loadFromFile(rom_path))
        return;
    // create the mapper based on the mapper ID in the iNES header of the ROM
    mapper = Mapper::create(
        cartridge,
        [&](){ picture_bus.update_mirroring(); }
    );
    bus.assign_mapper(mapper);
    picture_bus.assign_mapper(mapper);
}

void Emulator::DMA(uint8_t page) {
    // skip the DMA cycles on the CPU
    cpu.skipDMACycles();
    // do the DMA page change on the PPU
    ppu.doDMA(bus.get_page_pointer(page));
}

void Emulator::step(unsigned char action) {
    // write the controller state to player 1
    controller1.write_buttons(action);
    // approximate a frame
    for (int i = 0; i < STEPS_PER_FRAME; i++) {
        ppu.step(picture_bus);
        cpu.step(bus);
    }
}

void Emulator::backup() {
    backup_bus = bus;
    backup_picture_bus = picture_bus;
    backup_cpu = cpu;
    backup_ppu = ppu;
}

void Emulator::restore() {
    bus = backup_bus;
    picture_bus = backup_picture_bus;
    cpu = backup_cpu;
    ppu = backup_ppu;
}
