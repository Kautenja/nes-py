//  Program:      nes-py
//  File:         mapper_MMC5.cpp
//  Description:  An implementation of the MMC5 / ExROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "nes_emu/mappers/mapper_MMC5.hpp"

namespace NES {

namespace {

NES_Byte defaultNametableMapping(Cartridge* cartridge) {
    if (cartridge == nullptr)
        return 0x50;
    if (cartridge->getNameTableMirroring() == VERTICAL)
        return 0x44;
    return 0x50;
}

}  // namespace

std::size_t MapperMMC5::mmc5PRGRAMSize(Cartridge* cartridge) {
    if (cartridge == nullptr)
        return 0;
    const std::size_t parsed_size = cartridge->getPRGMemorySize();
    if (parsed_size == 0)
        return 0;
    // iNES MMC5 RAM sizes are often underspecified. A 64 KiB allocation is
    // the compatible commercial-game superset while preserving zero-RAM NES2
    // cartridges as RAM-less.
    return parsed_size < 0x10000 ? 0x10000 : parsed_size;
}

MapperMMC5::MapperMMC5(Cartridge* cart) :
    Mapper(cart),
    prg_mode(3),
    chr_mode(3),
    prg_ram_protect_1(0),
    prg_ram_protect_2(0),
    exram_mode(0),
    nametable_mapping(defaultNametableMapping(cart)),
    fill_tile(0),
    fill_attribute(0),
    prg_banks(),
    chr_sprite_banks(),
    chr_background_banks(),
    chr_upper_bits(0),
    background_banks_active(false),
    chr_memory(cart),
    exram(),
    name_table_ram(),
    background_chr_fetches_remaining(0),
    extended_attribute_latch(0),
    irq_scanline_compare(0),
    irq_enabled(false),
    irq_pending(false),
    in_frame(false),
    scanline_counter(0),
    scanline_tile_fetches(0),
    ppu_idle_cycles(3),
    ppu_read_seen(false),
    multiplier_a(0xff),
    multiplier_b(0xff),
    vertical_split_mode(0),
    vertical_split_scroll(0),
    vertical_split_bank(0) {
    resizePRGRAM(mmc5PRGRAMSize(cart));
    prg_banks.fill(0);
    prg_banks[4] = 0xff;
    chr_sprite_banks.fill(0);
    chr_background_banks.fill(0);
    exram.fill(0);
    name_table_ram.fill(0);
}

bool MapperMMC5::prgRAMWriteEnabled() const {
    return prg_ram_protect_1 == 0x02 && prg_ram_protect_2 == 0x01;
}

MapperMMC5::PRGWindow MapperMMC5::prgWindowFor(
    NES_Address address
) const {
    switch (prg_mode) {
        case 0:
            return {prg_banks[4], 0x8000, 0x8000, true};
        case 1:
            if (address < 0xc000)
                return {prg_banks[2], 0x8000, 0x4000, false};
            return {prg_banks[4], 0xc000, 0x4000, true};
        case 2:
            if (address < 0xc000)
                return {prg_banks[2], 0x8000, 0x4000, false};
            if (address < 0xe000)
                return {prg_banks[3], 0xc000, 0x2000, false};
            return {prg_banks[4], 0xe000, 0x2000, true};
        default:
            if (address < 0xa000)
                return {prg_banks[1], 0x8000, 0x2000, false};
            if (address < 0xc000)
                return {prg_banks[2], 0xa000, 0x2000, false};
            if (address < 0xe000)
                return {prg_banks[3], 0xc000, 0x2000, false};
            return {prg_banks[4], 0xe000, 0x2000, true};
    }
}

NES_Byte MapperMMC5::readPRGROMWindow(
    NES_Byte reg,
    NES_Address address,
    NES_Address window_base,
    std::size_t window_size
) const {
    const std::vector<NES_Byte>& rom = cartridge->getROM();
    if (rom.empty())
        return 0;

    const std::size_t span = window_size / PRG_BANK_SIZE;
    const std::size_t bank = MapperBank::maskBankSelect(
        reg & 0x7f,
        MapperBank::bankCount(rom.size(), PRG_BANK_SIZE),
        span
    );
    const std::size_t offset =
        (bank * PRG_BANK_SIZE) + ((address - window_base) % window_size);
    return rom[offset % rom.size()];
}

NES_Byte MapperMMC5::readPRGRAMWindow(
    NES_Byte reg,
    NES_Address address,
    NES_Address window_base,
    std::size_t window_size
) const {
    if (prg_ram.empty())
        return 0;

    const std::size_t span = window_size / PRG_BANK_SIZE;
    const std::size_t bank = MapperBank::maskBankSelect(
        reg & 0x0f,
        MapperBank::bankCount(prg_ram.size(), PRG_BANK_SIZE),
        span
    );
    const std::size_t offset =
        (bank * PRG_BANK_SIZE) + ((address - window_base) % window_size);
    return prg_ram[offset % prg_ram.size()];
}

void MapperMMC5::writePRGRAMWindow(
    NES_Byte reg,
    NES_Address address,
    NES_Address window_base,
    std::size_t window_size,
    NES_Byte value
) {
    if (!prgRAMWriteEnabled() || prg_ram.empty())
        return;

    const std::size_t span = window_size / PRG_BANK_SIZE;
    const std::size_t bank = MapperBank::maskBankSelect(
        reg & 0x0f,
        MapperBank::bankCount(prg_ram.size(), PRG_BANK_SIZE),
        span
    );
    const std::size_t offset =
        (bank * PRG_BANK_SIZE) + ((address - window_base) % window_size);
    prg_ram[offset % prg_ram.size()] = value;
}

NES_Byte MapperMMC5::readPRG(NES_Address address) {
    const PRGWindow window = prgWindowFor(address);
    if (window.force_rom || (window.reg & 0x80))
        return readPRGROMWindow(window.reg, address, window.base, window.size);
    return readPRGRAMWindow(window.reg, address, window.base, window.size);
}

void MapperMMC5::writePRG(NES_Address address, NES_Byte value) {
    const PRGWindow window = prgWindowFor(address);
    if (!window.force_rom && !(window.reg & 0x80)) {
        writePRGRAMWindow(
            window.reg,
            address,
            window.base,
            window.size,
            value
        );
    }
}

NES_Byte MapperMMC5::readPRGRAM(NES_Address address) {
    return readPRGRAMWindow(prg_banks[0], address, 0x6000, PRG_BANK_SIZE);
}

void MapperMMC5::writePRGRAM(NES_Address address, NES_Byte value) {
    writePRGRAMWindow(prg_banks[0], address, 0x6000, PRG_BANK_SIZE, value);
}

const NES_Byte* MapperMMC5::getPRGRAMPointer(NES_Address address) {
    if (prg_ram.empty())
        return nullptr;

    const std::size_t bank = MapperBank::maskBankSelect(
        prg_banks[0] & 0x0f,
        MapperBank::bankCount(prg_ram.size(), PRG_BANK_SIZE)
    );
    const std::size_t offset =
        (bank * PRG_BANK_SIZE) + ((address - 0x6000) % PRG_BANK_SIZE);
    return &prg_ram[offset % prg_ram.size()];
}

NES_Byte MapperMMC5::readCHRBank(
    std::uint16_t bank,
    std::size_t bank_size,
    NES_Address offset
) const {
    const std::vector<NES_Byte>& vrom = cartridge->getVROM();
    if (vrom.empty())
        return 0;

    const std::size_t bank_count = MapperBank::bankCount(
        vrom.size(),
        bank_size
    );
    if (bank_count == 0)
        return 0;
    const std::size_t selected_bank = bank % bank_count;
    const std::size_t resolved =
        (selected_bank * bank_size) + (offset % bank_size);
    return vrom[resolved % vrom.size()];
}

NES_Byte MapperMMC5::readSpriteCHR(NES_Address address) const {
    address &= 0x1fff;
    switch (chr_mode) {
        case 0:
            return readCHRBank(chr_sprite_banks[7], 0x2000, address);
        case 1:
            if (address < 0x1000)
                return readCHRBank(chr_sprite_banks[3], 0x1000, address);
            return readCHRBank(chr_sprite_banks[7], 0x1000, address & 0x0fff);
        case 2:
            if (address < 0x0800)
                return readCHRBank(chr_sprite_banks[1], 0x0800, address);
            if (address < 0x1000)
                return readCHRBank(chr_sprite_banks[3], 0x0800, address);
            if (address < 0x1800)
                return readCHRBank(chr_sprite_banks[5], 0x0800, address);
            return readCHRBank(chr_sprite_banks[7], 0x0800, address);
        default:
            return readCHRBank(
                chr_sprite_banks[(address >> 10) & 0x07],
                CHR_1K_BANK_SIZE,
                address
            );
    }
}

NES_Byte MapperMMC5::readBackgroundCHR(NES_Address address) const {
    address &= 0x1fff;
    if (exram_mode == 1) {
        const std::uint16_t bank = static_cast<std::uint16_t>(
            ((chr_upper_bits & 0x03) << 6) |
            (extended_attribute_latch & 0x3f)
        );
        return readCHRBank(bank, 0x1000, address & 0x0fff);
    }
    if (!background_banks_active)
        return readSpriteCHR(address);

    const NES_Address local = address & 0x0fff;
    switch (chr_mode) {
        case 0:
            return readCHRBank(chr_background_banks[3], 0x2000, address);
        case 1:
            return readCHRBank(chr_background_banks[3], 0x1000, local);
        case 2:
            if (local < 0x0800)
                return readCHRBank(chr_background_banks[1], 0x0800, local);
            return readCHRBank(chr_background_banks[3], 0x0800, local);
        default:
            return readCHRBank(
                chr_background_banks[(local >> 10) & 0x03],
                CHR_1K_BANK_SIZE,
                local
            );
    }
}

NES_Byte MapperMMC5::readCHR(NES_Address address) {
    markPPURead();
    if (chr_memory.usesRAM())
        return chr_memory.read(address);

    if (background_chr_fetches_remaining > 0) {
        --background_chr_fetches_remaining;
        return readBackgroundCHR(address);
    }
    return readSpriteCHR(address);
}

void MapperMMC5::writeCHR(NES_Address address, NES_Byte value) {
    markPPURead();
    if (chr_memory.usesRAM())
        chr_memory.write(address, value);
}

std::uint16_t MapperMMC5::multiplierProduct() const {
    return static_cast<std::uint16_t>(multiplier_a) * multiplier_b;
}

NES_Byte MapperMMC5::readExpansion(NES_Address address) {
    if (address == 0x5204) {
        NES_Byte status = 0;
        if (irq_pending)
            status |= 0x80;
        if (in_frame)
            status |= 0x40;
        irq_pending = false;
        return status;
    }
    if (address == 0x5205)
        return static_cast<NES_Byte>(multiplierProduct() & 0xff);
    if (address == 0x5206)
        return static_cast<NES_Byte>((multiplierProduct() >> 8) & 0xff);
    if (address >= 0x5c00 && address <= 0x5fff) {
        if (exram_mode == 2 || exram_mode == 3)
            return exram[address & 0x03ff];
        return 0;
    }
    return 0;
}

void MapperMMC5::writeExpansion(NES_Address address, NES_Byte value) {
    if (address >= 0x5c00 && address <= 0x5fff) {
        if (exram_mode != 3)
            exram[address & 0x03ff] = value;
        return;
    }

    if (address >= 0x5000 && address <= 0x5015) {
        // MMC5 pulse/PCM audio registers are intentionally unsupported in
        // this mapper pass; writes are accepted as no-ops for compatibility.
        return;
    }

    switch (address) {
        case 0x5100:
            prg_mode = value & 0x03;
            break;
        case 0x5101:
            chr_mode = value & 0x03;
            break;
        case 0x5102:
            prg_ram_protect_1 = value & 0x03;
            break;
        case 0x5103:
            prg_ram_protect_2 = value & 0x03;
            break;
        case 0x5104:
            exram_mode = value & 0x03;
            break;
        case 0x5105:
            nametable_mapping = value;
            break;
        case 0x5106:
            fill_tile = value;
            break;
        case 0x5107:
            fill_attribute = value & 0x03;
            break;
        case 0x5113:
        case 0x5114:
        case 0x5115:
        case 0x5116:
        case 0x5117:
            prg_banks[address - 0x5113] = value;
            break;
        case 0x5120:
        case 0x5121:
        case 0x5122:
        case 0x5123:
        case 0x5124:
        case 0x5125:
        case 0x5126:
        case 0x5127:
            chr_sprite_banks[address - 0x5120] =
                static_cast<std::uint16_t>((chr_upper_bits << 8) | value);
            break;
        case 0x5128:
        case 0x5129:
        case 0x512a:
        case 0x512b:
            chr_background_banks[address - 0x5128] =
                static_cast<std::uint16_t>((chr_upper_bits << 8) | value);
            background_banks_active = true;
            break;
        case 0x5130:
            chr_upper_bits = value & 0x03;
            break;
        case 0x5200:
            vertical_split_mode = value;
            break;
        case 0x5201:
            vertical_split_scroll = value;
            break;
        case 0x5202:
            vertical_split_bank = value;
            break;
        case 0x5203:
            irq_scanline_compare = value;
            break;
        case 0x5204:
            irq_enabled = (value & 0x80) != 0;
            if (irq_enabled && irq_pending)
                requestIRQ();
            break;
        case 0x5205:
            multiplier_a = value;
            break;
        case 0x5206:
            multiplier_b = value;
            break;
        default:
            break;
    }
}

bool MapperMMC5::handlesExpansion(NES_Address address) const {
    return address >= 0x5000 && address <= 0x5fff;
}

void MapperMMC5::markPPURead() {
    ppu_read_seen = true;
    ppu_idle_cycles = 0;
}

void MapperMMC5::detectScanline() {
    if (!in_frame) {
        in_frame = true;
        scanline_counter = 0;
        return;
    }

    ++scanline_counter;
    if (irq_scanline_compare != 0 && scanline_counter == irq_scanline_compare) {
        irq_pending = true;
        if (irq_enabled)
            requestIRQ();
    }
}

void MapperMMC5::onCPUCycle() {
    if (ppu_read_seen) {
        ppu_idle_cycles = 0;
    } else if (ppu_idle_cycles < 3) {
        ++ppu_idle_cycles;
        if (ppu_idle_cycles == 3) {
            in_frame = false;
            scanline_tile_fetches = 0;
            background_chr_fetches_remaining = 0;
        }
    }
    ppu_read_seen = false;
}

bool MapperMMC5::mapsNameTable(NES_Address address) const {
    address &= 0x3fff;
    return address >= 0x2000 && address < 0x3f00;
}

bool MapperMMC5::isAttributeAddress(NES_Address address) {
    return (address & 0x03ff) >= 0x03c0;
}

NES_Byte MapperMMC5::fillAttributeByte() const {
    const NES_Byte palette = fill_attribute & 0x03;
    return palette | (palette << 2) | (palette << 4) | (palette << 6);
}

NES_Byte MapperMMC5::readNameTable(NES_Address address) {
    markPPURead();
    const NES_Address offset = address & 0x0fff;
    const NES_Address in_table = offset & 0x03ff;
    const std::size_t table = (offset >> 10) & 0x03;

    if (!isAttributeAddress(in_table)) {
        extended_attribute_latch = exram[in_table];
        background_chr_fetches_remaining = 2;
        if (scanline_tile_fetches == 0)
            detectScanline();
        scanline_tile_fetches = (scanline_tile_fetches + 1) & 0x1f;
    } else if (exram_mode == 1) {
        const NES_Byte palette = (extended_attribute_latch >> 6) & 0x03;
        return palette | (palette << 2) | (palette << 4) | (palette << 6);
    }

    const NES_Byte source = (nametable_mapping >> (table * 2)) & 0x03;
    switch (source) {
        case 0:
            return name_table_ram[in_table];
        case 1:
            return name_table_ram[0x0400 + in_table];
        case 2:
            if (exram_mode <= 1)
                return exram[in_table];
            return 0;
        default:
            return isAttributeAddress(in_table) ?
                fillAttributeByte() :
                fill_tile;
    }
}

void MapperMMC5::writeNameTable(NES_Address address, NES_Byte value) {
    markPPURead();
    const NES_Address offset = address & 0x0fff;
    const NES_Address in_table = offset & 0x03ff;
    const std::size_t table = (offset >> 10) & 0x03;
    const NES_Byte source = (nametable_mapping >> (table * 2)) & 0x03;

    switch (source) {
        case 0:
            name_table_ram[in_table] = value;
            break;
        case 1:
            name_table_ram[0x0400 + in_table] = value;
            break;
        case 2:
            if (exram_mode <= 1)
                exram[in_table] = value;
            break;
        default:
            break;
    }
}

}  // namespace NES
