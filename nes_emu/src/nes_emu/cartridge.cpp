//  Program:      nes-py
//  File:         cartridge.cpp
//  Description:  This class houses the logic and data for an NES cartridge
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include <fstream>
#include <stdexcept>
#include <string>
#include "nes_emu/cartridge.hpp"
#include "nes_emu/log.hpp"

namespace NES {

namespace {

const std::size_t HEADER_SIZE = 0x10;
const std::size_t TRAINER_SIZE = 0x200;
const std::size_t PRG_ROM_BANK_SIZE = 0x4000;
const std::size_t CHR_ROM_BANK_SIZE = 0x2000;
const std::size_t PRG_RAM_BANK_SIZE = 0x2000;
const std::size_t NES2_RAM_GRANULARITY = 64;

std::size_t decodeNES2ROMSize(
    NES_Byte low_byte,
    NES_Byte high_nibble,
    std::size_t unit
) {
    if (high_nibble == 0x0f) {
        std::size_t exponent = low_byte >> 2;
        std::size_t multiplier = (low_byte & 0x03) * 2 + 1;
        return (static_cast<std::size_t>(1) << exponent) * multiplier;
    }
    return (((static_cast<std::size_t>(high_nibble) << 8) | low_byte) * unit);
}

std::size_t decodeNES2RAMSize(NES_Byte shift_count) {
    if (shift_count == 0)
        return 0;
    return NES2_RAM_GRANULARITY << shift_count;
}

bool hasZeroFill(const std::vector<NES_Byte>& header) {
    for (std::size_t index = 11; index < header.size(); ++index) {
        if (header[index] != 0)
            return false;
    }
    return true;
}

void readExact(
    std::ifstream& rom_file,
    std::vector<NES_Byte>& target,
    const std::string& error_message
) {
    if (target.empty())
        return;

    rom_file.read(
        reinterpret_cast<char*>(&target[0]),
        static_cast<std::streamsize>(target.size())
    );
    if (rom_file.gcount() != static_cast<std::streamsize>(target.size()))
        throw std::runtime_error(error_message);
}

}  // namespace

void Cartridge::loadFromFile(
    std::string path,
    bool allow_unsupported_features
) {
    // create a stream to load the ROM file
    std::ifstream romFile(path, std::ios_base::binary | std::ios_base::in);
    if (!romFile)
        throw std::runtime_error("failed to open ROM file: " + path);

    // create a byte vector for the iNES header
    std::vector<NES_Byte> header;
    header.resize(HEADER_SIZE);
    romFile.read(
        reinterpret_cast<char*>(&header[0]),
        static_cast<std::streamsize>(HEADER_SIZE)
    );
    if (romFile.gcount() != static_cast<std::streamsize>(HEADER_SIZE))
        throw std::runtime_error("ROM header is truncated.");

    if (
        header[0] != 0x4e ||
        header[1] != 0x45 ||
        header[2] != 0x53 ||
        header[3] != 0x1a
    ) {
        throw std::runtime_error("ROM missing magic number in header.");
    }

    metadata = CartridgeMetadata();
    metadata.is_nes2 = (header[7] & 0x0c) == 0x08;
    if (!metadata.is_nes2 && !hasZeroFill(header))
        throw std::runtime_error("ROM header zero fill bytes are not zero.");

    metadata.has_trainer = (header[6] & 0x04) != 0;
    metadata.has_vs_unisystem = (header[7] & 0x01) != 0;
    metadata.has_play_choice_10 = (header[7] & 0x02) != 0;
    metadata.trainer_start = HEADER_SIZE;
    metadata.trainer_stop = HEADER_SIZE + (
        metadata.has_trainer ? TRAINER_SIZE : 0
    );

    metadata.mapper_number = ((header[6] >> 4) & 0x0f) | (header[7] & 0xf0);
    if (metadata.is_nes2) {
        metadata.mapper_number |= (
            static_cast<std::uint16_t>(header[8] & 0x0f) << 8
        );
        metadata.submapper = (header[8] >> 4) & 0x0f;
    }

    if (header[6] & 0x08) {
        metadata.name_table_mirroring = CARTRIDGE_MIRROR_FOUR_SCREEN;
    } else if (header[6] & 0x01) {
        metadata.name_table_mirroring = CARTRIDGE_MIRROR_VERTICAL;
    } else {
        metadata.name_table_mirroring = CARTRIDGE_MIRROR_HORIZONTAL;
    }

    if (metadata.is_nes2) {
        metadata.prg_rom_size = decodeNES2ROMSize(
            header[4],
            header[9] & 0x0f,
            PRG_ROM_BANK_SIZE
        );
        metadata.chr_rom_size = decodeNES2ROMSize(
            header[5],
            (header[9] >> 4) & 0x0f,
            CHR_ROM_BANK_SIZE
        );
        metadata.prg_ram_size = decodeNES2RAMSize(header[10] & 0x0f);
        metadata.prg_battery_ram_size = decodeNES2RAMSize(
            (header[10] >> 4) & 0x0f
        );
        metadata.chr_ram_size = decodeNES2RAMSize(header[11] & 0x0f);
        metadata.chr_battery_ram_size = decodeNES2RAMSize(
            (header[11] >> 4) & 0x0f
        );
        metadata.is_pal = (header[12] & 0x03) == 0x01;
    } else {
        metadata.prg_rom_size = header[4] * PRG_ROM_BANK_SIZE;
        metadata.chr_rom_size = header[5] * CHR_ROM_BANK_SIZE;
        metadata.prg_ram_size = (header[8] ? header[8] : 1) * PRG_RAM_BANK_SIZE;
        metadata.prg_battery_ram_size = (
            (header[6] & 0x02) ? metadata.prg_ram_size : 0
        );
        metadata.chr_ram_size = metadata.chr_rom_size == 0 ? CHR_ROM_BANK_SIZE : 0;
        metadata.chr_battery_ram_size = 0;
        metadata.is_pal = (header[9] & 0x01) != 0;
    }

    metadata.prg_rom_banks = metadata.prg_rom_size / PRG_ROM_BANK_SIZE;
    metadata.chr_rom_banks = metadata.chr_rom_size / CHR_ROM_BANK_SIZE;
    metadata.has_battery = (
        (header[6] & 0x02) ||
        metadata.prg_battery_ram_size ||
        metadata.chr_battery_ram_size
    );

    std::vector<NES_Byte> trainer;
    trainer.resize(metadata.has_trainer ? TRAINER_SIZE : 0);
    readExact(romFile, trainer, "failed to read trainer on ROM.");

    if (metadata.has_trainer && !allow_unsupported_features)
        throw std::runtime_error("ROM has trainer. trainer is not supported.");
    if (metadata.is_pal && !allow_unsupported_features)
        throw std::runtime_error("ROM is PAL. PAL is not supported.");
    if (metadata.prg_rom_size == 0)
        throw std::runtime_error("ROM has no PRG-ROM banks.");

    prg_rom.clear();
    prg_rom.resize(metadata.prg_rom_size);
    readExact(romFile, prg_rom, "failed to read PRG-ROM on ROM.");

    chr_rom.clear();
    chr_rom.resize(metadata.chr_rom_size);
    readExact(romFile, chr_rom, "failed to read CHR-ROM on ROM.");
}

}  // namespace NES
