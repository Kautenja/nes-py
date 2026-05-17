//  Program:      nes-py
//  File:         mapper_MMC5.hpp
//  Description:  An implementation of the MMC5 / ExROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef MAPPERMMC5_HPP
#define MAPPERMMC5_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include "nes_emu/common.hpp"
#include "nes_emu/mapper.hpp"
#include "nes_emu/mapper_bank.hpp"

namespace NES {

/// Nintendo MMC5 / ExROM mapper implementation.
class MapperMMC5 : public Mapper {
 private:
    static const std::size_t PRG_BANK_SIZE = 0x2000;
    static const std::size_t CHR_1K_BANK_SIZE = 0x0400;
    static const std::size_t EXRAM_SIZE = 0x0400;
    static const std::size_t CIRAM_SIZE = 0x0800;

    struct PRGWindow {
        NES_Byte reg;
        NES_Address base;
        std::size_t size;
        bool force_rom;
    };

    /// PRG banking mode from $5100.
    NES_Byte prg_mode;
    /// CHR banking mode from $5101.
    NES_Byte chr_mode;
    /// PRG RAM protection keys from $5102/$5103.
    NES_Byte prg_ram_protect_1;
    NES_Byte prg_ram_protect_2;
    /// ExRAM mode from $5104.
    NES_Byte exram_mode;
    /// Nametable source selector from $5105.
    NES_Byte nametable_mapping;
    /// Fill-mode tile and attribute selector from $5106/$5107.
    NES_Byte fill_tile;
    NES_Byte fill_attribute;
    /// PRG bank registers $5113-$5117.
    std::array<NES_Byte, 5> prg_banks;
    /// Sprite/normal CHR bank registers $5120-$5127.
    std::array<std::uint16_t, 8> chr_sprite_banks;
    /// Background CHR bank registers $5128-$512B.
    std::array<std::uint16_t, 4> chr_background_banks;
    /// Upper CHR bank bits from $5130 for subsequent CHR writes.
    NES_Byte chr_upper_bits;
    /// Whether background CHR registers have been explicitly initialized.
    bool background_banks_active;
    /// Optional writable CHR RAM.
    MapperBank::CHRMemory chr_memory;
    /// MMC5 internal ExRAM.
    std::array<NES_Byte, EXRAM_SIZE> exram;
    /// Mapper-owned CIRAM backing for arbitrary MMC5 nametable layouts.
    std::array<NES_Byte, CIRAM_SIZE> name_table_ram;
    /// Number of background pattern bytes expected after a nametable tile read.
    int background_chr_fetches_remaining;
    /// Latched extended attribute byte for the current background tile.
    NES_Byte extended_attribute_latch;

    /// Scanline IRQ state from $5203/$5204 and PPU fetch observations.
    NES_Byte irq_scanline_compare;
    bool irq_enabled;
    bool irq_pending;
    bool in_frame;
    NES_Byte scanline_counter;
    int scanline_tile_fetches;
    int ppu_idle_cycles;
    bool ppu_read_seen;

    /// Multiplier write registers $5205/$5206.
    NES_Byte multiplier_a;
    NES_Byte multiplier_b;

    /// Vertical split registers are preserved but not rendered in this pass.
    NES_Byte vertical_split_mode;
    NES_Byte vertical_split_scroll;
    NES_Byte vertical_split_bank;

    /// Return the compatible MMC5 PRG RAM allocation size.
    static std::size_t mmc5PRGRAMSize(Cartridge* cartridge);

    /// Return whether PRG RAM writes are currently enabled.
    bool prgRAMWriteEnabled() const;

    /// Return the PRG window descriptor for a CPU address.
    PRGWindow prgWindowFor(NES_Address address) const;

    /// Read or write the banked PRG RAM window.
    NES_Byte readPRGRAMWindow(
        NES_Byte reg,
        NES_Address address,
        NES_Address window_base,
        std::size_t window_size
    ) const;
    void writePRGRAMWindow(
        NES_Byte reg,
        NES_Address address,
        NES_Address window_base,
        std::size_t window_size,
        NES_Byte value
    );

    /// Read a banked PRG ROM window.
    NES_Byte readPRGROMWindow(
        NES_Byte reg,
        NES_Address address,
        NES_Address window_base,
        std::size_t window_size
    ) const;

    /// Return a byte from CHR ROM using an MMC5 bank value and size.
    NES_Byte readCHRBank(
        std::uint16_t bank,
        std::size_t bank_size,
        NES_Address offset
    ) const;

    /// Read CHR data through sprite/normal or background bank registers.
    NES_Byte readSpriteCHR(NES_Address address) const;
    NES_Byte readBackgroundCHR(NES_Address address) const;

    /// Return the current multiplier product.
    std::uint16_t multiplierProduct() const;

    /// Update IRQ state after a synthetic scanline is detected.
    void detectScanline();

    /// Mark that the PPU read through this mapper during the current CPU cycle.
    void markPPURead();

    /// Return the fill-mode attribute byte.
    NES_Byte fillAttributeByte() const;

    /// Return true for nametable attribute addresses.
    static bool isAttributeAddress(NES_Address address);

 public:
    /// Create a new mapper with a cartridge.
    explicit MapperMMC5(Cartridge* cart);

    /// Return a copy of this mapper and its current state.
    inline std::unique_ptr<Mapper> clone() const {
        return std::unique_ptr<Mapper>(new MapperMMC5(*this));
    }

    /// Read a byte from CPU $8000-$FFFF.
    NES_Byte readPRG(NES_Address address);

    /// Write a byte to CPU $8000-$FFFF.
    void writePRG(NES_Address address, NES_Byte value);

    /// Read a byte from PPU CHR space.
    NES_Byte readCHR(NES_Address address);

    /// Write a byte to PPU CHR space.
    void writeCHR(NES_Address address, NES_Byte value);

    /// Read a byte from banked PRG RAM at CPU $6000-$7FFF.
    NES_Byte readPRGRAM(NES_Address address);

    /// Write a byte to banked PRG RAM at CPU $6000-$7FFF.
    void writePRGRAM(NES_Address address, NES_Byte value);

    /// Return a DMA pointer for the selected PRG RAM bank, if present.
    const NES_Byte* getPRGRAMPointer(NES_Address address);

    /// Read a byte from CPU $5000-$5FFF expansion/register space.
    NES_Byte readExpansion(NES_Address address);

    /// Write a byte to CPU $5000-$5FFF expansion/register space.
    void writeExpansion(NES_Address address, NES_Byte value);

    /// Return true when the MMC5 handles this expansion address.
    bool handlesExpansion(NES_Address address) const;

    /// Observe CPU cycles for MMC5 scanline in-frame expiry.
    void onCPUCycle();

    /// Return true because MMC5 IRQ status tracks PPU-read idleness.
    inline bool observesCPUCycles() const { return true; }

    /// MMC5 owns nametable routing for arbitrary $5105 layouts.
    inline bool hasNameTableMapping() const { return true; }

    /// Return true for PPU nametable addresses handled by MMC5.
    bool mapsNameTable(NES_Address address) const;

    /// Read mapper-owned or mapper-remapped nametable data.
    NES_Byte readNameTable(NES_Address address);

    /// Write mapper-owned or mapper-remapped nametable data.
    void writeNameTable(NES_Address address, NES_Byte value);
};

}  // namespace NES

#endif  // MAPPERMMC5_HPP
