//  Program:      nes-py
//  File:         mapper_MMC3.cpp
//  Description:  An implementation of the MMC3 mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "nes_emu/mappers/mapper_MMC3.hpp"

namespace NES {

namespace {

const std::size_t PRG_WINDOW_SIZE = 0x2000;
const std::size_t CHR_WINDOW_SIZE = 0x0400;

}  // namespace

MapperMMC3::MapperMMC3(Cartridge* cart) :
    Mapper(cart),
    bank_select(0),
    bank_registers{{0, 2, 4, 5, 6, 7, 0, 1}},
    prg_mode(false),
    chr_inversion(false),
    prg_ram_enabled(true),
    chr_memory(cart),
    irq_latch(0),
    irq_counter(0),
    irq_reload(false),
    irq_enabled(false),
    last_ppu_a12(false),
    ppu_a12_low_observations(0) {
    setPRGRAMWritable(true);
    calculatePRGWindows();
    calculateCHRWindows();
}

void MapperMMC3::calculatePRGWindows() {
    const std::size_t rom_size = cartridge->getROM().size();
    const std::size_t bank_count = MapperBank::bankCount(
        rom_size,
        PRG_WINDOW_SIZE
    );
    const std::size_t second_last_bank = bank_count < 2 ? 0 : bank_count - 2;

    if (prg_mode) {
        prg_windows[0].selectBankIndex(
            rom_size,
            PRG_WINDOW_SIZE,
            second_last_bank
        );
        prg_windows[2].selectBank(
            rom_size,
            PRG_WINDOW_SIZE,
            bank_registers[6]
        );
    } else {
        prg_windows[0].selectBank(
            rom_size,
            PRG_WINDOW_SIZE,
            bank_registers[6]
        );
        prg_windows[2].selectBankIndex(
            rom_size,
            PRG_WINDOW_SIZE,
            second_last_bank
        );
    }

    prg_windows[1].selectBank(
        rom_size,
        PRG_WINDOW_SIZE,
        bank_registers[7]
    );
    prg_windows[3].selectFinal(rom_size, PRG_WINDOW_SIZE);
}

void MapperMMC3::calculateCHRWindows() {
    if (chr_memory.usesRAM())
        return;

    const std::size_t vrom_size = cartridge->getVROM().size();
    const NES_Byte first_2k = bank_registers[0] & 0xfe;
    const NES_Byte second_2k = bank_registers[1] & 0xfe;

    if (chr_inversion) {
        chr_windows[0].selectBank(vrom_size, CHR_WINDOW_SIZE, bank_registers[2]);
        chr_windows[1].selectBank(vrom_size, CHR_WINDOW_SIZE, bank_registers[3]);
        chr_windows[2].selectBank(vrom_size, CHR_WINDOW_SIZE, bank_registers[4]);
        chr_windows[3].selectBank(vrom_size, CHR_WINDOW_SIZE, bank_registers[5]);
        chr_windows[4].selectBank(vrom_size, CHR_WINDOW_SIZE, first_2k);
        chr_windows[5].selectBank(vrom_size, CHR_WINDOW_SIZE, first_2k + 1);
        chr_windows[6].selectBank(vrom_size, CHR_WINDOW_SIZE, second_2k);
        chr_windows[7].selectBank(vrom_size, CHR_WINDOW_SIZE, second_2k + 1);
    } else {
        chr_windows[0].selectBank(vrom_size, CHR_WINDOW_SIZE, first_2k);
        chr_windows[1].selectBank(vrom_size, CHR_WINDOW_SIZE, first_2k + 1);
        chr_windows[2].selectBank(vrom_size, CHR_WINDOW_SIZE, second_2k);
        chr_windows[3].selectBank(vrom_size, CHR_WINDOW_SIZE, second_2k + 1);
        chr_windows[4].selectBank(vrom_size, CHR_WINDOW_SIZE, bank_registers[2]);
        chr_windows[5].selectBank(vrom_size, CHR_WINDOW_SIZE, bank_registers[3]);
        chr_windows[6].selectBank(vrom_size, CHR_WINDOW_SIZE, bank_registers[4]);
        chr_windows[7].selectBank(vrom_size, CHR_WINDOW_SIZE, bank_registers[5]);
    }
}

NES_Byte MapperMMC3::readPRG(NES_Address address) {
    const std::size_t slot = (address - 0x8000) / PRG_WINDOW_SIZE;
    const NES_Address base = static_cast<NES_Address>(
        0x8000 + slot * PRG_WINDOW_SIZE
    );
    return prg_windows[slot].read(cartridge->getROM(), address, base);
}

void MapperMMC3::writePRG(NES_Address address, NES_Byte value) {
    const bool is_odd_register = (address & 0x0001) != 0;

    if (address <= 0x9fff) {
        if (is_odd_register) {
            NES_Byte selected_register = bank_select & 0x07;
            if (selected_register <= 1)
                value &= 0xfe;
            bank_registers[selected_register] = value;
            if (selected_register <= 5)
                calculateCHRWindows();
            else
                calculatePRGWindows();
        } else {
            bank_select = value & 0x07;
            prg_mode = (value & 0x40) != 0;
            chr_inversion = (value & 0x80) != 0;
            calculatePRGWindows();
            calculateCHRWindows();
        }
    } else if (address <= 0xbfff) {
        if (is_odd_register) {
            prg_ram_enabled = (value & 0x80) != 0;
            setPRGRAMWritable(prg_ram_enabled && ((value & 0x40) == 0));
        } else if (cartridge->getNameTableMirroring() != FOUR_SCREEN) {
            setNameTableMirroring((value & 0x01) ? HORIZONTAL : VERTICAL);
        }
    } else if (address <= 0xdfff) {
        if (is_odd_register) {
            irq_reload = true;
        } else {
            irq_latch = value;
        }
    } else {
        irq_enabled = is_odd_register;
    }
}

NES_Byte MapperMMC3::readCHR(NES_Address address) {
    if (chr_memory.usesRAM())
        return chr_memory.read(address);

    const std::size_t slot = (address & 0x1fff) / CHR_WINDOW_SIZE;
    const NES_Address base = static_cast<NES_Address>(slot * CHR_WINDOW_SIZE);
    return chr_windows[slot].read(cartridge->getVROM(), address, base);
}

const NES_Byte* MapperMMC3::getDirectCHRReadPage(NES_Address page_base) {
    if (chr_memory.usesRAM())
        return chr_memory.readPointer(page_base, CHR_WINDOW_SIZE);

    const std::size_t slot = (page_base & 0x1fff) / CHR_WINDOW_SIZE;
    const NES_Address base = static_cast<NES_Address>(slot * CHR_WINDOW_SIZE);
    return chr_windows[slot].readPointer(
        cartridge->getVROM(),
        page_base,
        base,
        CHR_WINDOW_SIZE
    );
}

void MapperMMC3::writeCHR(NES_Address address, NES_Byte value) {
    if (chr_memory.usesRAM())
        chr_memory.write(address, value);
}

NES_Byte MapperMMC3::readPRGRAM(NES_Address address) {
    if (!prg_ram_enabled)
        return 0;
    return Mapper::readPRGRAM(address);
}

void MapperMMC3::writePRGRAM(NES_Address address, NES_Byte value) {
    if (!prg_ram_enabled)
        return;
    Mapper::writePRGRAM(address, value);
}

const NES_Byte* MapperMMC3::getPRGRAMPointer(NES_Address address) {
    if (!prg_ram_enabled)
        return nullptr;
    return Mapper::getPRGRAMPointer(address);
}

void MapperMMC3::clockIRQCounter() {
    // Model the common MMC3B-compatible reload behavior: a filtered edge can
    // request an IRQ when reloading a zero latch leaves the counter at zero.
    if (irq_counter == 0 || irq_reload) {
        irq_counter = irq_latch;
        irq_reload = false;
    } else {
        --irq_counter;
    }

    if (irq_counter == 0 && irq_enabled)
        requestIRQ();
}

void MapperMMC3::onPPUAddress(NES_Address address) {
    const bool a12 = (address & 0x1000) != 0;

    if (!a12) {
        last_ppu_a12 = false;
        if (ppu_a12_low_observations < A12_LOW_FILTER_OBSERVATIONS)
            ++ppu_a12_low_observations;
        return;
    }

    if (!last_ppu_a12 &&
            ppu_a12_low_observations >= A12_LOW_FILTER_OBSERVATIONS) {
        clockIRQCounter();
    }
    last_ppu_a12 = true;
    ppu_a12_low_observations = 0;
}

bool MapperMMC3::invalidatesDirectPRGReadPagesOnWrite(
    NES_Address address,
    NES_Byte value
) const {
    (void) value;
    if (address > 0x9fff)
        return false;
    if ((address & 0x0001) == 0)
        return true;
    return (bank_select & 0x07) >= 6;
}

bool MapperMMC3::invalidatesDirectCHRReadPagesOnWrite(
    NES_Address address,
    NES_Byte value
) const {
    (void) value;
    if (address > 0x9fff)
        return false;
    if ((address & 0x0001) == 0)
        return true;
    return (bank_select & 0x07) <= 5;
}

}  // namespace NES
