//  Program:      nes-py
//  File:         mapper_NROM.cpp
//  Description:  An implementation of the NROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "mappers/mapper_NROM.hpp"
#include "log.hpp"

namespace NES {

MapperNROM::MapperNROM(Cartridge* cart) :
    Mapper(cart),
    is_one_bank(cart->getROM().size() == 0x4000),
    has_character_ram(cart->getVROM().size() == 0) {
    if (has_character_ram) {
        character_ram.resize(0x2000);
        LOG(Info) << "Uses character RAM" << std::endl;
    }
}

void MapperNROM::writePRG(NES_Address address, NES_Byte value) {
    LOG(InfoVerbose) <<
        "ROM memory write attempt at " <<
        +address <<
        " to set " <<
        +value <<
        std::endl;
}

void MapperNROM::writeCHR(NES_Address address, NES_Byte value) {
    if (has_character_ram)
        character_ram[address] = value;
    else
        LOG(Info) <<
            "Read-only CHR memory write attempt at " <<
            std::hex <<
            address <<
            std::endl;
}

}  // namespace NES
