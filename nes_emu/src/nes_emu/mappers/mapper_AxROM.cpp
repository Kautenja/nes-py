//  Program:      nes-py
//  File:         mapper_AxROM.cpp
//  Description:  An implementation of the AxROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "nes_emu/mappers/mapper_AxROM.hpp"

namespace NES {

MapperAxROM::MapperAxROM(Cartridge* cart) :
    Mapper(cart),
    chr_memory(cart),
    bus_conflicts(cart != nullptr && cart->getSubmapper() == 2) {
    selected_prg.selectFirst(cart->getROM().size(), 0x8000);
    chr_rom.selectFirst(cart->getVROM().size(), 0x2000);
    setNameTableMirroring(ONE_SCREEN_LOWER);
}

NES_Byte MapperAxROM::readPRG(NES_Address address) {
    return selected_prg.read(cartridge->getROM(), address, 0x8000);
}

void MapperAxROM::writePRG(NES_Address address, NES_Byte value) {
    (void) address;
    selected_prg.selectBank(cartridge->getROM().size(), 0x8000, value & 0x07);
    setNameTableMirroring(
        (value & 0x10) ? ONE_SCREEN_HIGHER : ONE_SCREEN_LOWER
    );
}

NES_Byte MapperAxROM::readCHR(NES_Address address) {
    if (chr_memory.usesRAM())
        return chr_memory.read(address);
    return chr_rom.read(cartridge->getVROM(), address, 0x0000);
}

void MapperAxROM::writeCHR(NES_Address address, NES_Byte value) {
    if (chr_memory.usesRAM())
        chr_memory.write(address, value);
}

}  // namespace NES
