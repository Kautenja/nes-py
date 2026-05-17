//  Program:      nes-py
//  File:         mapper_NROM.cpp
//  Description:  An implementation of the NROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "mappers/mapper_NROM.hpp"

namespace NES {

MapperNROM::MapperNROM(Cartridge* cart) :
    Mapper(cart),
    chr_memory(cart) {
    first_prg.selectFirst(cart->getROM().size(), 0x4000);
    second_prg.selectFinal(cart->getROM().size(), 0x4000);
    chr_rom.selectFirst(cart->getVROM().size(), 0x2000);
}

void MapperNROM::writePRG(NES_Address address, NES_Byte value) {
    (void) address;
    (void) value;
}

void MapperNROM::writeCHR(NES_Address address, NES_Byte value) {
    if (chr_memory.usesRAM())
        chr_memory.write(address, value);
}

}  // namespace NES
