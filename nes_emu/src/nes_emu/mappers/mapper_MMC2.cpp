//  Program:      nes-py
//  File:         mapper_MMC2.cpp
//  Description:  An implementation of the MMC2 mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "nes_emu/mappers/mapper_MMC2.hpp"

namespace NES {

namespace {

const std::size_t PRG_BANK_SIZE = 0x2000;
const std::size_t CHR_BANK_SIZE = 0x1000;

std::size_t fixedBank(
    std::size_t memory_size,
    std::size_t bank_from_end
) {
    const std::size_t count = MapperBank::bankCount(
        memory_size,
        PRG_BANK_SIZE
    );
    if (count <= bank_from_end)
        return 0;
    return count - bank_from_end;
}

}  // namespace

MapperMMC2::MapperMMC2(Cartridge* cart) :
    Mapper(cart),
    chr_memory(cart),
    register_prg(0),
    register_chr_left_fd(0),
    register_chr_left_fe(0),
    register_chr_right_fd(0),
    register_chr_right_fe(0),
    left_latch_fe(true),
    right_latch_fe(true),
    pending_latch(LATCH_NONE),
    pending_latch_address(0) {
    updatePRG();
    updateFixedPRG();
    updateLeftCHR();
    updateRightCHR();
}

void MapperMMC2::updatePRG() {
    switchable_prg.selectBank(
        cartridge->getROM().size(),
        PRG_BANK_SIZE,
        register_prg & 0x0f
    );
}

void MapperMMC2::updateFixedPRG() {
    const std::size_t rom_size = cartridge->getROM().size();
    fixed_prg_a.selectBankIndex(rom_size, PRG_BANK_SIZE, fixedBank(rom_size, 3));
    fixed_prg_b.selectBankIndex(rom_size, PRG_BANK_SIZE, fixedBank(rom_size, 2));
    fixed_prg_c.selectBankIndex(rom_size, PRG_BANK_SIZE, fixedBank(rom_size, 1));
}

void MapperMMC2::updateLeftCHR() {
    const NES_Byte bank = left_latch_fe ?
        register_chr_left_fe :
        register_chr_left_fd;
    left_chr.selectBank(
        cartridge->getVROM().size(),
        CHR_BANK_SIZE,
        bank & 0x1f
    );
}

void MapperMMC2::updateRightCHR() {
    const NES_Byte bank = right_latch_fe ?
        register_chr_right_fe :
        register_chr_right_fd;
    right_chr.selectBank(
        cartridge->getVROM().size(),
        CHR_BANK_SIZE,
        bank & 0x1f
    );
}

void MapperMMC2::applyPendingLatch(NES_Address address) {
    const PendingLatch latch = pending_latch;
    const NES_Address latch_address = pending_latch_address;
    pending_latch = LATCH_NONE;

    if (latch == LATCH_NONE || address != latch_address)
        return;

    switch (latch) {
        case LATCH_LEFT_FD:
            if (left_latch_fe) {
                left_latch_fe = false;
                updateLeftCHR();
            }
            break;
        case LATCH_LEFT_FE:
            if (!left_latch_fe) {
                left_latch_fe = true;
                updateLeftCHR();
            }
            break;
        case LATCH_RIGHT_FD:
            if (right_latch_fe) {
                right_latch_fe = false;
                updateRightCHR();
            }
            break;
        case LATCH_RIGHT_FE:
            if (!right_latch_fe) {
                right_latch_fe = true;
                updateRightCHR();
            }
            break;
        case LATCH_NONE:
            break;
    }
}

NES_Byte MapperMMC2::readPRG(NES_Address address) {
    if (address < 0xa000)
        return switchable_prg.read(cartridge->getROM(), address, 0x8000);
    if (address < 0xc000)
        return fixed_prg_a.read(cartridge->getROM(), address, 0xa000);
    if (address < 0xe000)
        return fixed_prg_b.read(cartridge->getROM(), address, 0xc000);
    return fixed_prg_c.read(cartridge->getROM(), address, 0xe000);
}

void MapperMMC2::writePRG(NES_Address address, NES_Byte value) {
    switch (address & 0xf000) {
        case 0xa000:
            register_prg = value & 0x0f;
            updatePRG();
            break;
        case 0xb000:
            register_chr_left_fd = value & 0x1f;
            if (!left_latch_fe)
                updateLeftCHR();
            break;
        case 0xc000:
            register_chr_left_fe = value & 0x1f;
            if (left_latch_fe)
                updateLeftCHR();
            break;
        case 0xd000:
            register_chr_right_fd = value & 0x1f;
            if (!right_latch_fe)
                updateRightCHR();
            break;
        case 0xe000:
            register_chr_right_fe = value & 0x1f;
            if (right_latch_fe)
                updateRightCHR();
            break;
        case 0xf000:
            setNameTableMirroring((value & 1) ? HORIZONTAL : VERTICAL);
            break;
        default:
            break;
    }
}

NES_Byte MapperMMC2::readCHR(NES_Address address) {
    NES_Byte value;
    if (chr_memory.usesRAM())
        value = chr_memory.read(address);
    else if (address < 0x1000)
        value = left_chr.read(cartridge->getVROM(), address, 0x0000);
    else
        value = right_chr.read(cartridge->getVROM(), address, 0x1000);
    applyPendingLatch(address);
    return value;
}

void MapperMMC2::writeCHR(NES_Address address, NES_Byte value) {
    if (chr_memory.usesRAM())
        chr_memory.write(address, value);
    pending_latch = LATCH_NONE;
}

void MapperMMC2::onPPUAddress(NES_Address address) {
    pending_latch_address = address;
    if (address == 0x0fd8) {
        pending_latch = LATCH_LEFT_FD;
    } else if (address == 0x0fe8) {
        pending_latch = LATCH_LEFT_FE;
    } else if (address >= 0x1fd8 && address <= 0x1fdf) {
        pending_latch = LATCH_RIGHT_FD;
    } else if (address >= 0x1fe8 && address <= 0x1fef) {
        pending_latch = LATCH_RIGHT_FE;
    } else {
        pending_latch = LATCH_NONE;
    }
}

}  // namespace NES
