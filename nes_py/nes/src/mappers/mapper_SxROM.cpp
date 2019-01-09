//  Program:      nes-py
//  File:         mapper_SxROM.cpp
//  Description:  An implementation of the SxROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "mappers/mapper_SxROM.hpp"
#include "log.hpp"

MapperSxROM::MapperSxROM(Cartridge &cart, std::function<void(void)> mirroring_cb) :
    Mapper(cart),
    mirroring_callback(mirroring_cb),
    mirroing(HORIZONTAL),
    mode_chr(0),
    mode_prg(3),
    temp_register(0),
    write_counter(0),
    register_prg(0),
    register_chr0(0),
    register_chr1(0),
    first_bank_prg(nullptr),
    second_bank_prg(nullptr),
    first_bank_chr(nullptr),
    second_bank_chr(nullptr) {
    if (cart.getVROM().size() == 0) {
        has_character_ram = true;
        character_ram.resize(0x2000);
        LOG(Info) << "Uses character RAM" << std::endl;
    }
    else {
        LOG(Info) << "Using CHR-ROM" << std::endl;
        has_character_ram = false;
        first_bank_chr = &cart.getVROM()[0];
        second_bank_chr = &cart.getVROM()[0x1000 * register_chr1];
    }

    first_bank_prg = &cart.getROM()[0]; //first bank
    second_bank_prg = &cart.getROM()[cart.getROM().size() - 0x4000/*0x2000 * 0x0e*/]; //last bank
}

NES_Byte MapperSxROM::readPRG(NES_Address address) {
    if (address < 0xc000)
        return *(first_bank_prg + (address & 0x3fff));
    else
        return *(second_bank_prg + (address & 0x3fff));
}

void MapperSxROM::writePRG(NES_Address address, NES_Byte value) {
    //if reset bit is NOT set
    if (!(value & 0x80))  {
        temp_register = (temp_register >> 1) | ((value & 1) << 4);
        ++write_counter;

        if (write_counter == 5) {
            if (address <= 0x9fff) {
                switch (temp_register & 0x3) {
                    case 0:     mirroing = ONE_SCREEN_LOWER;   break;
                    case 1:     mirroing = ONE_SCREEN_HIGHER;  break;
                    case 2:     mirroing = VERTICAL;           break;
                    case 3:     mirroing = HORIZONTAL;         break;
                }
                mirroring_callback();

                mode_chr = (temp_register & 0x10) >> 4;
                mode_prg = (temp_register & 0xc) >> 2;
                calculatePRGPointers();

                // Recalculate CHR pointers
                if (mode_chr == 0) { //one 8KB bank
                    first_bank_chr = &cartridge.getVROM()[0x1000 * (register_chr0 | 1)]; //ignore last bit
                    second_bank_chr = first_bank_chr + 0x1000;
                }
                else { // two 4KB banks
                    first_bank_chr = &cartridge.getVROM()[0x1000 * register_chr0];
                    second_bank_chr = &cartridge.getVROM()[0x1000 * register_chr1];
                }
            }
            else if (address <= 0xbfff) { // CHR Reg 0
                register_chr0 = temp_register;
                first_bank_chr = &cartridge.getVROM()[0x1000 * (temp_register | (1 - mode_chr))]; //OR 1 if 8KB mode
                if (mode_chr == 0)
                    second_bank_chr = first_bank_chr + 0x1000;
            }
            else if (address <= 0xdfff) {
                register_chr1 = temp_register;
                if(mode_chr == 1)
                    second_bank_chr = &cartridge.getVROM()[0x1000 * temp_register];
            }
            else {
                // TODO: PRG-RAM
                if ((temp_register & 0x10) == 0x10) {
                    LOG(Info) << "PRG-RAM activated" << std::endl;
                }

                temp_register &= 0xf;
                register_prg = temp_register;
                calculatePRGPointers();
            }

            temp_register = 0;
            write_counter = 0;
        }
    }
    else { // reset
        temp_register = 0;
        write_counter = 0;
        mode_prg = 3;
        calculatePRGPointers();
    }
}

void MapperSxROM::calculatePRGPointers() {
    if (mode_prg <= 1) { // 32KB changeable
        // equivalent to multiplying 0x8000 * (register_prg >> 1)
        first_bank_prg = &cartridge.getROM()[0x4000 * (register_prg & ~1)];
        second_bank_prg = first_bank_prg + 0x4000;   //add 16KB
    }
    else if (mode_prg == 2) { // fix first switch second
        first_bank_prg = &cartridge.getROM()[0];
        second_bank_prg = first_bank_prg + 0x4000 * register_prg;
    }
    else { // switch first fix second
        first_bank_prg = &cartridge.getROM()[0x4000 * register_prg];
        second_bank_prg = &cartridge.getROM()[cartridge.getROM().size() - 0x4000/*0x2000 * 0x0e*/];
    }
}

const NES_Byte* MapperSxROM::getPagePtr(NES_Address address) {
    if (address < 0xc000)
        return (first_bank_prg + (address & 0x3fff));
    else
        return (second_bank_prg + (address & 0x3fff));
}

NES_Byte MapperSxROM::readCHR(NES_Address address) {
    if (has_character_ram)
        return character_ram[address];
    else if (address < 0x1000)
        return *(first_bank_chr + address);
    else
        return *(second_bank_chr + (address & 0xfff));
}

void MapperSxROM::writeCHR(NES_Address address, NES_Byte value) {
    if (has_character_ram)
        character_ram[address] = value;
    else
        LOG(Info) << "Read-only CHR memory write attempt at " << std::hex << address << std::endl;
}
