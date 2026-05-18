//  Program:      nes-py
//  File:         picture_bus.hpp
//  Description:  This class houses picture bus data from the PPU
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef PICTURE_BUS_HPP
#define PICTURE_BUS_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include "nes_emu/common.hpp"
#include "nes_emu/mapper.hpp"

namespace NES {

/// The bus for graphical data to travel along
class PictureBus {
 private:
    /// Number of bytes in local nametable RAM, including four-screen RAM.
    static const std::size_t NAME_TABLE_RAM_SIZE = 0x1000;
    /// Number of bytes in local palette RAM.
    static const std::size_t PALETTE_RAM_SIZE = 0x20;
    /// Number of direct CHR read pages mapped at PPU $0000-$1fff.
    static const std::size_t DIRECT_CHR_READ_PAGE_COUNT = 8;
    /// Byte size of each direct CHR read page.
    static const std::size_t DIRECT_CHR_READ_PAGE_SIZE = 0x0400;
    /// the VRAM on the picture bus
    std::array<NES_Byte, NAME_TABLE_RAM_SIZE> ram;
    /// indexes where they start in RAM vector
    std::array<std::size_t, 4> name_tables;
    /// the palette for decoding RGB tuples
    std::array<NES_Byte, PALETTE_RAM_SIZE> palette;
    /// a pointer to the mapper on the cartridge
    Mapper* mapper;
    /// cached mapper capability flags for hot-path PPU dispatch
    bool mapper_observes_ppu_addresses;
    bool mapper_observes_ppu_reads;
    bool mapper_observes_ppu_writes;
    bool mapper_has_name_table_mapping;
    bool mapper_allows_sprite_row_prefetch;
    /// Direct CHR page pointers for mapper PPU read hot paths.
    std::array<const NES_Byte*, DIRECT_CHR_READ_PAGE_COUNT> direct_chr_read_pages;
    /// Incremented whenever CPU/PPU writes can invalidate cached render data.
    std::uint64_t write_generation;

    /// Normalize palette addresses, including universal background mirrors.
    static inline NES_Byte normalize_palette_address(NES_Address address) {
        NES_Byte palette_address = static_cast<NES_Byte>(address & 0x1f);
        if ((palette_address & 0x10) && ((palette_address & 0x03) == 0))
            palette_address &= 0x0f;
        return palette_address;
    }

 public:
    /// Mutable picture-bus state captured by backup/restore.
    struct State {
        std::array<NES_Byte, NAME_TABLE_RAM_SIZE> ram;
        std::array<NES_Byte, PALETTE_RAM_SIZE> palette;
    };

    /// Initialize a new picture bus.
    PictureBus() :
        ram(),
        name_tables{{0, 0, 0, 0}},
        palette(),
        mapper(nullptr),
        mapper_observes_ppu_addresses(false),
        mapper_observes_ppu_reads(false),
        mapper_observes_ppu_writes(false),
        mapper_has_name_table_mapping(false),
        mapper_allows_sprite_row_prefetch(false),
        direct_chr_read_pages{{
            nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr, nullptr, nullptr
        }},
        write_generation(0) { }

    /// Read a byte from an address on the VRAM.
    ///
    /// @param address the 16-bit address of the byte to read in the VRAM
    ///
    /// @return the byte located at the given address
    ///
    NES_Byte read(NES_Address address);

    /// Write a byte to an address in the VRAM.
    ///
    /// @param address the 16-bit address to write the byte to in VRAM
    /// @param value the byte to write to the given address
    ///
    void write(NES_Address address, NES_Byte value);

    /// Set the mapper pointer to a new value.
    ///
    /// @param mapper the new mapper pointer for the bus to use
    ///
    inline void set_mapper(Mapper *mapper) {
        this->mapper = mapper;
        mapper_observes_ppu_addresses = (
            mapper != nullptr && mapper->observesPPUAddresses()
        );
        mapper_observes_ppu_reads = (
            mapper != nullptr && mapper->observesPPUReads()
        );
        mapper_observes_ppu_writes = (
            mapper != nullptr && mapper->observesPPUWrites()
        );
        mapper_has_name_table_mapping = (
            mapper != nullptr && mapper->hasNameTableMapping()
        );
        mapper_allows_sprite_row_prefetch = (
            mapper != nullptr && mapper->allowsSpriteRowPrefetch()
        );
        refresh_direct_chr_read_pages();
        update_mirroring();
    }

    /// Return a copy of mutable picture-bus state without mapper wiring.
    inline State save_state() const { return {ram, palette}; }

    /// Restore mutable picture-bus state without changing mapper wiring.
    inline void load_state(const State& state) {
        ram = state.ram;
        palette = state.palette;
        ++write_generation;
    }

    /// Return true when mapper-visible PPU hooks require uncached bus reads.
    inline bool has_mapper_ppu_observers() const {
        return (
            mapper_observes_ppu_addresses ||
            mapper_observes_ppu_reads ||
            mapper_observes_ppu_writes
        );
    }

    /// Return true when sprite rows can be prefetched without mapper effects.
    inline bool can_prefetch_sprite_rows() const {
        return (
            mapper != nullptr &&
            mapper_allows_sprite_row_prefetch &&
            !has_mapper_ppu_observers() &&
            !mapper_has_name_table_mapping
        );
    }

    /// Return the current write generation for render-cache invalidation.
    inline std::uint64_t get_write_generation() const {
        return write_generation;
    }

    /// Read a color index from the palette.
    ///
    /// @param address the address of the palette color
    ///
    /// @return the index of the RGB tuple in the color array
    ///
    inline NES_Byte read_palette(NES_Byte address) const {
        return palette[normalize_palette_address(address)];
    }

    /// Update the mirroring and name table from the mapper.
    void update_mirroring();

    /// Refresh direct CHR read pages from the active mapper.
    void refresh_direct_chr_read_pages();
};

}  // namespace NES

#endif  // PICTURE_BUS_HPP
