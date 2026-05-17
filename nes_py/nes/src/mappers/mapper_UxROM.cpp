//  Program:      nes-py
//  File:         mapper_UxROM.cpp
//  Description:  An implementation of the UxROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "mappers/mapper_UxROM.hpp"

namespace NES {

MapperUxROM::MapperUxROM(Cartridge* cart) :
    Mapper(cart),
    chr_memory(cart) {
    switchable_prg.selectFirst(cart->getROM().size(), 0x4000);
    fixed_prg.selectFinal(cart->getROM().size(), 0x4000);
    chr_rom.selectFirst(cart->getVROM().size(), 0x2000);
}

NES_Byte MapperUxROM::readPRG(NES_Address address) {
    if (address < 0xc000)
        return switchable_prg.read(cartridge->getROM(), address, 0x8000);
    return fixed_prg.read(cartridge->getROM(), address, 0xc000);
}

void MapperUxROM::writePRG(NES_Address address, NES_Byte value) {
    (void) address;
    // This implementation models no bus conflicts; MainBus resolves conflicts
    // first if a mapper opts into them through hasBusConflicts().
    switchable_prg.selectBank(cartridge->getROM().size(), 0x4000, value);
}

NES_Byte MapperUxROM::readCHR(NES_Address address) {
    if (chr_memory.usesRAM())
        return chr_memory.read(address);
    return chr_rom.read(cartridge->getVROM(), address, 0x0000);
}

void MapperUxROM::writeCHR(NES_Address address, NES_Byte value) {
    if (chr_memory.usesRAM())
        chr_memory.write(address, value);
}

}  // namespace NES
