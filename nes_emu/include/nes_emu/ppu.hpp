//  Program:      nes-py
//  File:         ppu.hpp
//  Description:  This class houses the logic and data for the PPU of an NES
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef PPU_HPP
#define PPU_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include "nes_emu/common.hpp"
#include "nes_emu/picture_bus.hpp"

namespace NES {

/// The number of visible scan lines (i.e., the height of the screen)
const int VISIBLE_SCANLINES = 240;
/// The number of visible dots per scan line (i.e., the width of the screen)
const int SCANLINE_VISIBLE_DOTS = 256;
/// The number of cycles per scanline
const int SCANLINE_CYCLE_LENGTH = 341;
/// The last cycle of a scan line (changed from 340 to fix render glitch)
const int SCANLINE_END_CYCLE = 341;
/// The last scanline per frame
const int FRAME_END_SCANLINE = 261;

/// The Picture Processing Unit (PPU) for the NES
class PPU {
 private:
    /// Number of bytes in object attribute memory.
    static const std::size_t OAM_SIZE = 64 * 4;
    /// Maximum sprites considered on one scanline by NES sprite evaluation.
    static const std::size_t MAX_SCANLINE_SPRITES = 8;
    /// Minimum sprite pressure needed before row prefetch amortizes its cost.
    static const std::size_t SPRITE_PREFETCH_MIN_SPRITES = 2;
    /// Number of pixels in the visible screen buffer.
    static const std::size_t SCREEN_PIXEL_COUNT =
        VISIBLE_SCANLINES * SCANLINE_VISIBLE_DOTS;
    /// The callback to fire when entering vertical blanking mode
    std::function<void(void)> vblank_callback;
    /// The OAM memory (sprites)
    std::array<NES_Byte, OAM_SIZE> sprite_memory;
    /// Sprite indexes selected for the next scanline.
    std::array<NES_Byte, MAX_SCANLINE_SPRITES> scanline_sprites;
    /// Number of valid entries in scanline_sprites.
    std::size_t scanline_sprite_count;
    /// Cached sprite pattern row ready for the current scanline.
    struct SpriteRow {
        NES_Byte sprite_index;
        NES_Byte x;
        NES_Byte attribute;
        NES_Byte pattern_low;
        NES_Byte pattern_high;
        NES_Byte palette_base;
        std::array<NES_Byte, 8> pixels;
        bool foreground;
        bool valid;

        SpriteRow() :
            sprite_index(0),
            x(0),
            attribute(0),
            pattern_low(0),
            pattern_high(0),
            palette_base(0),
            pixels(),
            foreground(false),
            valid(false) { }
    };
    /// Decoded sprite rows selected for the current scanline.
    std::array<SpriteRow, MAX_SCANLINE_SPRITES> scanline_sprite_rows;
    /// Whether scanline_sprite_rows can be used for current composition.
    bool scanline_sprite_rows_cached;
    /// Picture-bus write generation captured when rows were prefetched.
    std::uint64_t scanline_sprite_rows_generation;

    /// The current pipeline state of the PPU
    enum PipelineState {
        PRE_RENDER,
        RENDER,
        POST_RENDER,
        VERTICAL_BLANK
    } pipeline_state;

    /// The number of cycles left in the frame
    int cycles;
    /// the current scanline of the frame
    int scanline;
    /// whether the PPU is on an even frame
    bool is_even_frame;

    // Status

    /// whether the PPU is in vertical blanking mode
    bool is_vblank;
    /// whether sprite 0 has been hit (i.e., collision detection)
    bool is_sprite_zero_hit;

    // Registers

    /// the current data address to (read / write) (from / to)
    NES_Address data_address;
    /// a temporary address register
    NES_Address temp_address;
    /// the fine scrolling position
    NES_Byte fine_x_scroll;
    /// TODO: doc
    bool is_first_write;
    /// The address of the data buffer
    NES_Byte data_buffer;
    /// the read / write address for the OAM memory (sprites)
    NES_Byte sprite_data_address;

    // Mask

    /// whether the PPU is showing sprites
    bool is_showing_sprites;
    /// whether the PPU is showing background pixels
    bool is_showing_background;
    /// whether the PPU is hiding sprites along the edges
    bool is_hiding_edge_sprites;
    /// whether the PPU is hiding the background along the edges
    bool is_hiding_edge_background;

    // Setup flags and variables

    /// TODO: doc
    bool is_long_sprites;
    /// whether the PPU is in the interrupt handler
    bool is_interrupting;

    /// TODO: doc
    enum CharacterPage {
        LOW,
        HIGH,
    } background_page, sprite_page;

    /// The value to increment the data address by
    NES_Address data_address_increment;
    /// Whether background tile-row cache contains valid fetched data.
    bool background_tile_cache_valid;
    /// PPU address used for the cached background tile-row.
    NES_Address background_tile_cache_address;
    /// Pattern table page used for the cached background tile-row.
    CharacterPage background_tile_cache_page;
    /// Picture-bus write generation used for the cached background tile-row.
    std::uint64_t background_tile_cache_generation;
    /// Cached low pattern byte for the current background tile row.
    NES_Byte background_tile_cache_low;
    /// Cached high pattern byte for the current background tile row.
    NES_Byte background_tile_cache_high;
    /// Cached attribute byte for the current background tile row.
    NES_Byte background_tile_cache_attribute;
    /// Cached low bitplane with bits indexed by fine X.
    NES_Byte background_tile_cache_low_bits;
    /// Cached high bitplane with bits indexed by fine X.
    NES_Byte background_tile_cache_high_bits;
    /// Cached high palette bits for the current attribute quadrant.
    NES_Byte background_tile_cache_palette_high;
    /// Cached opacity mask indexed by fine X.
    NES_Byte background_tile_cache_opaque_mask;

    /// The internal screen data structure with height matching the visible
    /// scan lines and width matching the number of visible scan line dots.
    NES_Pixel screen[VISIBLE_SCANLINES][SCANLINE_VISIBLE_DOTS];

    /// Invalidate sprite row prefetch data.
    void invalidate_sprite_rows();

    /// Return the pattern address for a sprite row after vertical selection.
    NES_Address sprite_pattern_address(NES_Byte tile, int y_offset) const;

    /// Emit mapper-visible sprite fetches that the simplified renderer does
    /// not otherwise need for pixel composition.
    void observe_dummy_sprite_fetch(PictureBus& bus) const;

    /// Prefetch rows for sprites selected for the next scanline.
    void prefetch_scanline_sprite_rows(PictureBus& bus);

    /// Evaluate visible sprites for the next scanline.
    void evaluate_scanline_sprites(PictureBus& bus);

    /// Invalidate decoded background tile-row data.
    void invalidate_background_tile_cache();

    /// Decode and cache the current background tile row.
    void fill_background_tile_cache(
        PictureBus& bus,
        std::uint64_t generation
    );

 public:
    /// Mutable PPU state captured by backup/restore.
    struct Snapshot {
        std::array<NES_Byte, OAM_SIZE> sprite_memory;
        std::array<NES_Byte, MAX_SCANLINE_SPRITES> scanline_sprites;
        std::size_t scanline_sprite_count;
        std::array<SpriteRow, MAX_SCANLINE_SPRITES> scanline_sprite_rows;
        bool scanline_sprite_rows_cached;
        std::uint64_t scanline_sprite_rows_generation;
        int pipeline_state;
        int cycles;
        int scanline;
        bool is_even_frame;
        bool is_vblank;
        bool is_sprite_zero_hit;
        NES_Address data_address;
        NES_Address temp_address;
        NES_Byte fine_x_scroll;
        bool is_first_write;
        NES_Byte data_buffer;
        NES_Byte sprite_data_address;
        bool is_showing_sprites;
        bool is_showing_background;
        bool is_hiding_edge_sprites;
        bool is_hiding_edge_background;
        bool is_long_sprites;
        bool is_interrupting;
        int background_page;
        int sprite_page;
        NES_Address data_address_increment;
        bool background_tile_cache_valid;
        NES_Address background_tile_cache_address;
        int background_tile_cache_page;
        std::uint64_t background_tile_cache_generation;
        NES_Byte background_tile_cache_low;
        NES_Byte background_tile_cache_high;
        NES_Byte background_tile_cache_attribute;
        NES_Byte background_tile_cache_low_bits;
        NES_Byte background_tile_cache_high_bits;
        NES_Byte background_tile_cache_palette_high;
        NES_Byte background_tile_cache_opaque_mask;
        std::array<NES_Pixel, SCREEN_PIXEL_COUNT> screen;
    };

    /// Initialize a new PPU.
    PPU() :
        sprite_memory(),
        scanline_sprites(),
        scanline_sprite_rows(),
        scanline_sprite_count(0),
        scanline_sprite_rows_cached(false),
        scanline_sprite_rows_generation(0),
        background_tile_cache_low_bits(0),
        background_tile_cache_high_bits(0),
        background_tile_cache_palette_high(0),
        background_tile_cache_opaque_mask(0),
        screen() { }

    /// Perform a single cycle on the PPU.
    void cycle(PictureBus& bus);

    /// Reset the PPU.
    void reset();

    /// Return a copy of mutable PPU state without callback wiring.
    Snapshot save_state() const;

    /// Restore mutable PPU state without replacing callback wiring.
    void load_state(const Snapshot& snapshot);

    /// Set the interrupt callback for the CPU.
    inline void set_interrupt_callback(std::function<void(void)> cb) {
        vblank_callback = cb;
    }

    /// TODO: doc
    void do_DMA(const NES_Byte* page_ptr);

    // MARK: Callbacks mapped to CPU address space

    /// Set the control register to a new value.
    ///
    /// @param ctrl the new control register byte
    ///
    void control(NES_Byte ctrl);

    /// Set the mask register to a new value.
    ///
    /// @param mask the new mask value
    ///
    void set_mask(NES_Byte mask);

    /// Set the scroll register to a new value.
    ///
    /// @param scroll the new scroll register value
    ///
    void set_scroll(NES_Byte scroll);

    /// Return the value in the PPU status register.
    NES_Byte get_status();

    /// TODO: doc
    void set_data_address(NES_Byte address);

    /// Read data off the picture bus.
    ///
    /// @param bus the bus to read data off of
    ///
    NES_Byte get_data(PictureBus& bus);

    /// TODO: doc
    void set_data(PictureBus& bus, NES_Byte data);

    /// Set the sprite data address to a new value.
    ///
    /// @param address the new OAM data address
    ///
    inline void set_OAM_address(NES_Byte address) {
        sprite_data_address = address;
    }

    /// Read a byte from OAM memory at the sprite data address.
    ///
    /// @return the byte at the given address in OAM memory
    ///
    inline NES_Byte get_OAM_data() {
        return sprite_memory[sprite_data_address];
    }

    /// Write a byte to OAM memory at the sprite data address.
    ///
    /// @param value the byte to write to the given address
    ///
    inline void set_OAM_data(NES_Byte value) {
        sprite_memory[sprite_data_address++] = value;
        invalidate_sprite_rows();
    }

    /// Return a pointer to the screen buffer.
    inline NES_Pixel* get_screen_buffer() { return *screen; }
};

}  // namespace NES

#endif  // PPU_HPP
