//  Program:      nes-py
//  File:         emulator.cpp
//  Description:  This class houses the logic and data for an NES emulator
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "emulator.hpp"
#include "mapper_factory.hpp"
#include "log.hpp"
#include <stdexcept>

namespace NES {

Emulator::Emulator(std::string rom_path) :
    mapper_observes_cpu_cycles(false) {
    // set the read callbacks
    bus.set_read_callback(PPUSTATUS, [&](void) { return ppu.get_status();          });
    bus.set_read_callback(PPUDATA,   [&](void) { return ppu.get_data(picture_bus); });
    bus.set_read_callback(JOY1,      [&](void) { return controllers[0].read();     });
    bus.set_read_callback(JOY2,      [&](void) { return controllers[1].read();     });
    bus.set_read_callback(OAMDATA,   [&](void) { return ppu.get_OAM_data();        });
    // set the write callbacks
    bus.set_write_callback(PPUCTRL,  [&](NES_Byte b) { ppu.control(b);                                             });
    bus.set_write_callback(PPUMASK,  [&](NES_Byte b) { ppu.set_mask(b);                                            });
    bus.set_write_callback(OAMADDR,  [&](NES_Byte b) { ppu.set_OAM_address(b);                                     });
    bus.set_write_callback(PPUADDR,  [&](NES_Byte b) { ppu.set_data_address(b);                                    });
    bus.set_write_callback(PPUSCROL, [&](NES_Byte b) { ppu.set_scroll(b);                                          });
    bus.set_write_callback(PPUDATA,  [&](NES_Byte b) { ppu.set_data(picture_bus, b);                               });
    bus.set_write_callback(OAMDMA,   [&](NES_Byte b) { cpu.skip_DMA_cycles(); ppu.do_DMA(bus.get_page_pointer(b)); });
    bus.set_write_callback(JOY1,     [&](NES_Byte b) { controllers[0].strobe(b); controllers[1].strobe(b);         });
    bus.set_write_callback(OAMDATA,  [&](NES_Byte b) { ppu.set_OAM_data(b);                                        });
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

NES_Byte Emulator::read_prg(NES_Address address) {
    if (address < 0x6000)
        return mapper->readExpansion(address);
    if (address < 0x8000)
        return mapper->readPRGRAM(address);
    return mapper->readPRG(address);
}

void Emulator::write_prg(NES_Address address, NES_Byte value) {
    if (address < 0x6000) {
        mapper->writeExpansion(address, value);
    } else if (address < 0x8000) {
        mapper->writePRGRAM(address, value);
    } else {
        mapper->writePRG(address, mapper->resolveBusConflict(address, value));
    }
    synchronize_mapper_mirroring();
}

}  // namespace NES
