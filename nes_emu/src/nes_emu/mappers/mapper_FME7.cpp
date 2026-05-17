//  Program:      nes-py
//  File:         mapper_FME7.cpp
//  Description:  An implementation of the Sunsoft FME-7 / 5B mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "nes_emu/mappers/mapper_FME7.hpp"

namespace NES {

MapperFME7::MapperFME7(Cartridge* cart) :
    Mapper(cart),
    selected_command(0),
    prg_6000_control(0),
    prg_registers{{0, 1, 2}},
    chr_registers{{0, 1, 2, 3, 4, 5, 6, 7}},
    chr_ram(
        cart != nullptr && cart->getVROM().empty() && cart->getCHRRAMSize() > 0 ?
            cart->getCHRRAMSize() :
            0,
        0
    ),
    irq_counter_enabled(false),
    irq_enabled(false),
    irq_pending(false),
    irq_counter(0),
    audio_register_select(0),
    audio_registers{{0}} {
    updatePRG6000Window();
    for (std::size_t index = 0; index < PRG_WINDOW_COUNT; ++index)
        updatePRGWindow(index);
    fixed_prg.selectFinal(cart->getROM().size(), PRG_WINDOW_SIZE);
    for (std::size_t index = 0; index < CHR_WINDOW_COUNT; ++index)
        updateCHRWindow(index);
}

bool MapperFME7::prg6000SelectsRAM() const {
    return (prg_6000_control & 0x40) != 0;
}

bool MapperFME7::prg6000RAMEnabled() const {
    return (prg_6000_control & 0x80) != 0;
}

void MapperFME7::updatePRG6000Window() {
    if (prg6000SelectsRAM()) {
        prg_6000_window.selectBank(
            prg_ram.size(),
            PRG_WINDOW_SIZE,
            prg_6000_control & 0x3f
        );
    } else {
        prg_6000_window.selectBank(
            cartridge->getROM().size(),
            PRG_WINDOW_SIZE,
            prg_6000_control & 0x3f
        );
    }
}

void MapperFME7::updatePRGWindow(std::size_t index) {
    prg_windows[index].selectBank(
        cartridge->getROM().size(),
        PRG_WINDOW_SIZE,
        prg_registers[index] & 0x3f
    );
}

void MapperFME7::updateCHRWindow(std::size_t index) {
    const std::size_t memory_size = chr_ram.empty() ?
        cartridge->getVROM().size() :
        chr_ram.size();
    chr_windows[index].selectBank(
        memory_size,
        CHR_WINDOW_SIZE,
        chr_registers[index]
    );
}

void MapperFME7::applyCommand(NES_Byte value) {
    if (selected_command < 0x08) {
        chr_registers[selected_command] = value;
        updateCHRWindow(selected_command);
        return;
    }

    if (selected_command == 0x08) {
        prg_6000_control = value;
        updatePRG6000Window();
        return;
    }

    if (selected_command >= 0x09 && selected_command <= 0x0b) {
        const std::size_t index = selected_command - 0x09;
        prg_registers[index] = value;
        updatePRGWindow(index);
        return;
    }

    if (selected_command == 0x0c) {
        switch (value & 0x03) {
            case 0: { setNameTableMirroring(VERTICAL);          break; }
            case 1: { setNameTableMirroring(HORIZONTAL);        break; }
            case 2: { setNameTableMirroring(ONE_SCREEN_LOWER);  break; }
            case 3: { setNameTableMirroring(ONE_SCREEN_HIGHER); break; }
        }
        return;
    }

    if (selected_command == 0x0d) {
        irq_pending = false;
        irq_enabled = (value & 0x01) != 0;
        irq_counter_enabled = (value & 0x80) != 0;
        return;
    }

    if (selected_command == 0x0e) {
        irq_counter = static_cast<std::uint16_t>(
            (irq_counter & 0xff00) | value
        );
        return;
    }

    if (selected_command == 0x0f) {
        irq_counter = static_cast<std::uint16_t>(
            (irq_counter & 0x00ff) |
            (static_cast<std::uint16_t>(value) << 8)
        );
    }
}

NES_Byte MapperFME7::readPRGRAM(NES_Address address) {
    if (prg6000SelectsRAM()) {
        if (!prg6000RAMEnabled() || prg_ram.empty())
            return 0;
        return prg_ram[
            prg_6000_window.offset(address, 0x6000) % prg_ram.size()
        ];
    }
    return prg_6000_window.read(cartridge->getROM(), address, 0x6000);
}

void MapperFME7::writePRGRAM(NES_Address address, NES_Byte value) {
    if (!prg6000SelectsRAM() || !prg6000RAMEnabled() || prg_ram.empty())
        return;
    prg_ram[
        prg_6000_window.offset(address, 0x6000) % prg_ram.size()
    ] = value;
}

const NES_Byte* MapperFME7::getPRGRAMPointer(NES_Address address) {
    if (!prg6000SelectsRAM() || !prg6000RAMEnabled() || prg_ram.empty())
        return nullptr;
    return &prg_ram[
        prg_6000_window.offset(address, 0x6000) % prg_ram.size()
    ];
}

NES_Byte MapperFME7::readPRG(NES_Address address) {
    if (address < 0xa000)
        return prg_windows[0].read(cartridge->getROM(), address, 0x8000);
    if (address < 0xc000)
        return prg_windows[1].read(cartridge->getROM(), address, 0xa000);
    if (address < 0xe000)
        return prg_windows[2].read(cartridge->getROM(), address, 0xc000);
    return fixed_prg.read(cartridge->getROM(), address, 0xe000);
}

void MapperFME7::writePRG(NES_Address address, NES_Byte value) {
    if (address < 0xa000) {
        selected_command = value & 0x0f;
    } else if (address < 0xc000) {
        applyCommand(value);
    } else if (address < 0xe000) {
        // Sunsoft 5B audio register select. nes-py has no expansion-audio
        // mixer path yet, but preserving writes keeps save states complete.
        audio_register_select = value & 0x0f;
    } else {
        audio_registers[audio_register_select] = value;
    }
}

NES_Byte MapperFME7::readCHR(NES_Address address) {
    const std::size_t index = (address >> 10) & 0x07;
    const NES_Address base = static_cast<NES_Address>(index * CHR_WINDOW_SIZE);
    if (!chr_ram.empty()) {
        return chr_ram[
            chr_windows[index].offset(address, base) % chr_ram.size()
        ];
    }
    return chr_windows[index].read(cartridge->getVROM(), address, base);
}

void MapperFME7::writeCHR(NES_Address address, NES_Byte value) {
    if (chr_ram.empty())
        return;
    const std::size_t index = (address >> 10) & 0x07;
    const NES_Address base = static_cast<NES_Address>(index * CHR_WINDOW_SIZE);
    chr_ram[
        chr_windows[index].offset(address, base) % chr_ram.size()
    ] = value;
}

void MapperFME7::onCPUCycle() {
    if (!irq_counter_enabled)
        return;

    const std::uint16_t previous = irq_counter;
    --irq_counter;
    if (previous == 0 && irq_enabled && !irq_pending) {
        irq_pending = true;
        requestIRQ();
    }
}

}  // namespace NES
