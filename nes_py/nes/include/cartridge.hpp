//  Program:      nes-py
//  File:         cartridge.hpp
//  Description:  This class houses the logic and data for an NES cartridge
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef CARTRIDGE_HPP
#define CARTRIDGE_HPP

#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>
#include "common.hpp"

namespace NES {

/// Header-level mirroring modes stored on a cartridge.
enum CartridgeMirroring : NES_Byte {
    CARTRIDGE_MIRROR_HORIZONTAL = 0,
    CARTRIDGE_MIRROR_VERTICAL = 1,
    CARTRIDGE_MIRROR_FOUR_SCREEN = 8,
};

/// Parsed iNES or NES 2.0 cartridge metadata.
struct CartridgeMetadata {
    std::uint16_t mapper_number;
    NES_Byte submapper;
    std::size_t prg_rom_size;
    std::size_t prg_rom_banks;
    std::size_t chr_rom_size;
    std::size_t chr_rom_banks;
    std::size_t prg_ram_size;
    std::size_t prg_battery_ram_size;
    std::size_t chr_ram_size;
    std::size_t chr_battery_ram_size;
    bool has_trainer;
    bool has_battery;
    bool has_vs_unisystem;
    bool has_play_choice_10;
    bool is_pal;
    bool is_nes2;
    std::size_t trainer_start;
    std::size_t trainer_stop;
    NES_Byte name_table_mirroring;

    CartridgeMetadata() :
        mapper_number(0),
        submapper(0),
        prg_rom_size(0),
        prg_rom_banks(0),
        chr_rom_size(0),
        chr_rom_banks(0),
        prg_ram_size(0),
        prg_battery_ram_size(0),
        chr_ram_size(0),
        chr_battery_ram_size(0),
        has_trainer(false),
        has_battery(false),
        has_vs_unisystem(false),
        has_play_choice_10(false),
        is_pal(false),
        is_nes2(false),
        trainer_start(0x10),
        trainer_stop(0x10),
        name_table_mirroring(CARTRIDGE_MIRROR_HORIZONTAL) { }
};

/// A cartridge holding game ROM and a special hardware mapper emulation
class Cartridge {
 private:
    /// the PRG ROM
    std::vector<NES_Byte> prg_rom;
    /// the CHR ROM
    std::vector<NES_Byte> chr_rom;
    /// parsed cartridge metadata
    CartridgeMetadata metadata;

 public:
    /// Initialize a new cartridge
    Cartridge() : metadata() { }

    /// Return the ROM data.
    const inline std::vector<NES_Byte>& getROM() const { return prg_rom; }

    /// Return the VROM data.
    const inline std::vector<NES_Byte>& getVROM() const { return chr_rom; }

    /// Return the parsed cartridge metadata.
    const inline CartridgeMetadata& getMetadata() const { return metadata; }

    /// Return the mapper ID number.
    inline std::uint16_t getMapper() const { return metadata.mapper_number; }

    /// Return the NES 2.0 submapper number, or 0 for iNES.
    inline NES_Byte getSubmapper() const { return metadata.submapper; }

    /// Return the name table mirroring mode.
    inline NES_Byte getNameTableMirroring() const {
        return metadata.name_table_mirroring;
    }

    /// Return a boolean determining whether this cartridge uses extended RAM.
    inline bool hasExtendedRAM() const { return getPRGMemorySize() > 0; }

    /// Return the ordinary PRG RAM size in bytes.
    inline std::size_t getPRGRAMSize() const { return metadata.prg_ram_size; }

    /// Return the battery-backed PRG RAM size in bytes.
    inline std::size_t getPRGBatteryRAMSize() const {
        return metadata.prg_battery_ram_size;
    }

    /// Return the total directly-addressable PRG RAM allocation in bytes.
    inline std::size_t getPRGMemorySize() const {
        if (metadata.prg_ram_size > metadata.prg_battery_ram_size)
            return metadata.prg_ram_size;
        return metadata.prg_battery_ram_size;
    }

    /// Return the ordinary CHR RAM size in bytes.
    inline std::size_t getCHRRAMSize() const { return metadata.chr_ram_size; }

    /// Return the battery-backed CHR RAM size in bytes.
    inline std::size_t getCHRBatteryRAMSize() const {
        return metadata.chr_battery_ram_size;
    }

    /// Load a ROM file into the cartridge and build the corresponding mapper.
    void loadFromFile(
        std::string path,
        bool allow_unsupported_features = false
    );
};

}  // namespace NES

#endif // CARTRIDGE_HPP
