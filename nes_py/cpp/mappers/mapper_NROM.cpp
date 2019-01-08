//  Program:      nes-py
//  File:         mapper_NROM.cpp
//  Description:  An implementation of the NROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "mappers/mapper_NROM.hpp"
#include "log.hpp"

MapperNROM::MapperNROM(Cartridge &cart) : Mapper(cart) {
    if (cart.getROM().size() == 0x4000) // 1 bank
        is_one_bank = true;
    else // 2 banks
        is_one_bank = false;

    if (cart.getVROM().size() == 0) {
        has_character_ram = true;
        character_ram.resize(0x2000);
        LOG(Info) << "Uses character RAM" << std::endl;
    }
    else
        has_character_ram = false;
}

NES_Byte MapperNROM::readPRG(NES_Address address) {
    if (!is_one_bank)
        return cartridge.getROM()[address - 0x8000];
    else //mirrored
        return cartridge.getROM()[(address - 0x8000) & 0x3fff];
}

void MapperNROM::writePRG(NES_Address address, NES_Byte value) {
    LOG(InfoVerbose) <<
        "ROM memory write attempt at " <<
        +address <<
        " to set " <<
        +value <<
        std::endl;
}

NES_Byte MapperNROM::readCHR(NES_Address address) {
    if (has_character_ram)
        return character_ram[address];
    else
        return cartridge.getVROM()[address];
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

const NES_Byte* MapperNROM::getPagePtr(NES_Address address) {
    if (!is_one_bank)
        return &cartridge.getROM()[address - 0x8000];
    else //mirrored
        return &cartridge.getROM()[(address - 0x8000) & 0x3fff];
}
