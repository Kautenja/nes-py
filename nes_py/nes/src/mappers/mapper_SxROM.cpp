//  Program:      nes-py
//  File:         mapper_SxROM.cpp
//  Description:  An implementation of the SxROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "mappers/mapper_SxROM.hpp"

namespace NES {

MapperSxROM::MapperSxROM(Cartridge* cart) :
    Mapper(cart),
    mode_chr(0),
    mode_prg(3),
    temp_register(0),
    write_counter(0),
    register_prg(0),
    register_chr0(0),
    register_chr1(0),
    chr_memory(cart) {
    calculatePRGWindows();
    calculateCHRWindows();
}

void MapperSxROM::writePRG(NES_Address address, NES_Byte value) {
    if (!(value & 0x80)) {  // reset bit is NOT set
        temp_register = (temp_register >> 1) | ((value & 1) << 4);
        ++write_counter;

        if (write_counter == 5) {
            if (address <= 0x9fff) {
                switch (temp_register & 0x3) {
                    case 0: { setNameTableMirroring(ONE_SCREEN_LOWER);  break; }
                    case 1: { setNameTableMirroring(ONE_SCREEN_HIGHER); break; }
                    case 2: { setNameTableMirroring(VERTICAL);          break; }
                    case 3: { setNameTableMirroring(HORIZONTAL);        break; }
                }

                mode_chr = (temp_register & 0x10) >> 4;
                mode_prg = (temp_register & 0xc) >> 2;
                calculatePRGWindows();
                calculateCHRWindows();
            } else if (address <= 0xbfff) {  // CHR Reg 0
                register_chr0 = temp_register;
                calculateCHRWindows();
            } else if (address <= 0xdfff) {
                register_chr1 = temp_register;
                calculateCHRWindows();
            } else {
                setPRGRAMWritable((temp_register & 0x10) == 0);
                register_prg = temp_register & 0xf;
                calculatePRGWindows();
            }

            temp_register = 0;
            write_counter = 0;
        }
    } else {  // reset
        temp_register = 0;
        write_counter = 0;
        mode_prg = 3;
        calculatePRGWindows();
    }
}

void MapperSxROM::calculatePRGWindows() {
    const std::size_t rom_size = cartridge->getROM().size();
    if (mode_prg <= 1) {  // 32KB changeable
        first_prg.selectBank(rom_size, 0x4000, register_prg, 2);
        second_prg.selectBankIndex(rom_size, 0x4000, first_prg.bank() + 1);
    } else if (mode_prg == 2) {  // fix first switch second
        first_prg.selectFirst(rom_size, 0x4000);
        second_prg.selectBank(rom_size, 0x4000, register_prg);
    } else {  // switch first fix second
        first_prg.selectBank(rom_size, 0x4000, register_prg);
        second_prg.selectFinal(rom_size, 0x4000);
    }
}

void MapperSxROM::calculateCHRWindows() {
    if (chr_memory.usesRAM())
        return;

    const std::size_t vrom_size = cartridge->getVROM().size();
    if (mode_chr == 0) {  // one 8KB bank; low register bit is ignored.
        first_chr.selectBank(vrom_size, 0x1000, register_chr0, 2);
        second_chr.selectBankIndex(vrom_size, 0x1000, first_chr.bank() + 1);
    } else {  // two independent 4KB banks.
        first_chr.selectBank(vrom_size, 0x1000, register_chr0);
        second_chr.selectBank(vrom_size, 0x1000, register_chr1);
    }
}

void MapperSxROM::writeCHR(NES_Address address, NES_Byte value) {
    if (chr_memory.usesRAM())
        chr_memory.write(address, value);
}

}  // namespace NES
