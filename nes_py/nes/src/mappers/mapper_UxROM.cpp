//  Program:      nes-py
//  File:         mapper_UxROM.cpp
//  Description:  An implementation of the UxROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "mappers/mapper_UxROM.hpp"
#include "log.hpp"

MapperUxROM::MapperUxROM(Cartridge &cart) :
    Mapper(cart),
    select_prg(0) {
    if (cart.getVROM().size() == 0) {
        has_character_ram = true;
        character_ram.resize(0x2000);
        LOG(Info) << "Uses character RAM" << std::endl;
    }
    else
        has_character_ram = false;

    // last - 16KB
    last_bank_pointer = &cart.getROM()[cart.getROM().size() - 0x4000];
}

NES_Byte MapperUxROM::readPRG(NES_Address address) {
    if (address < 0xc000)
        return cartridge.getROM()[((address - 0x8000) & 0x3fff) | (select_prg << 14)];
    else
        return *(last_bank_pointer + (address & 0x3fff));
}

const NES_Byte* MapperUxROM::getPagePtr(NES_Address address) {
    if (address < 0xc000)
        return &cartridge.getROM()[((address - 0x8000) & 0x3fff) | (select_prg << 14)];
    else
        return last_bank_pointer + (address & 0x3fff);
}

NES_Byte MapperUxROM::readCHR(NES_Address address) {
    if (has_character_ram)
        return character_ram[address];
    else
        return cartridge.getVROM()[address];
}

void MapperUxROM::writeCHR(NES_Address address, NES_Byte value) {
    if (has_character_ram)
        character_ram[address] = value;
    else
        LOG(Info) <<
            "Read-only CHR memory write attempt at " <<
            std::hex <<
            address <<
            std::endl;
}
