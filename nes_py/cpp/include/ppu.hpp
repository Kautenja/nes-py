//  Program:      nes-py
//  File:         ppu.hpp
//  Description:  This class houses the logic and data for the PPU of an NES
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef PPU_HPP
#define PPU_HPP

#include "common.hpp"
#include "picture_bus.hpp"
#include "main_bus.hpp"

/// The number of visible scan lines (i.e., the height of the screen)
const int VISIBLE_SCANLINES = 240;
/// The number of visible dots per scan line (i.e., the width of the screen)
const int SCANLINE_VISIBLE_DOTS = 256;
/// The number of cycles per scanline
const int SCANLINE_CYCLE_LENGTH = 341;
/// The last cycle of a scan line
const int SCANLINE_END_CYCLE = 340;
/// The last scanline per frame
const int FRAME_END_SCANLINE = 261;

/// The Picture Processing Unit (PPU) for the NES
class PPU {

private:
    /// The callback to fire when entering vertical blanking mode
    std::function<void(void)> vblank_callback;
    /// The OAM memory (sprites)
    std::vector<NES_Byte> sprite_memory;
    /// OAM memory (sprites) for the next scanline
    std::vector<NES_Byte> scanline_sprites;

    /// The current pipeline state of the PPU
    enum State {
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

    /// whether the PPU is in vertical blanking mode
    bool is_vblank;
    /// whether sprite 0 has been hit (i.e., collision detection)
    bool is_sprite_zero_hit;

    // Registers

    NES_Address data_address;
    NES_Address temp_address;
    NES_Byte fine_x_scroll;
    bool is_first_write;
    NES_Byte data_buffer;

    NES_Byte sprite_data_address;

    // Setup flags and variables

    /// TODO: doc
    bool is_long_sprites;
    /// whether the PPU is in the interrupt handler
    bool is_interrupting;

    /// whether the PPU is showing sprites
    bool is_showing_sprites;
    /// whether the PPU is showing background pixels
    bool is_showing_background;
    /// whether the PPU is hiding sprites along the edges
    bool is_hiding_edge_sprites;
    /// whether the PPU is hiding the background along the edges
    bool is_hiding_edge_background;

    enum CharacterPage {
        LOW,
        HIGH,
    } background_page, sprite_page;

    NES_Address data_address_increment;

    /// The internal screen data structure as a vector representation of a
    /// matrix of height matching the visible scans lines and width matching
    /// the number of visible scan line dots
    NES_Pixel screen[VISIBLE_SCANLINES][SCANLINE_VISIBLE_DOTS];

    /// Read a byte from OAM memory.
    ///
    /// @param address the address to read from OAM memory
    /// @return the byte at the given address in OAM memory
    ///
    inline NES_Byte read_OAM(NES_Byte address) { return sprite_memory[address]; };

    /// Write a byte to OAM memory.
    ///
    /// @param address the address to write to in OAM memory
    /// @param value the byte to write to the given address
    ///
    inline void write_OAM(NES_Byte address, NES_Byte value) { sprite_memory[address] = value; };

public:
    /// Initialize a new PPU
    PPU() : sprite_memory(64 * 4) { };

    /// Perform a single cycle on the PPU
    void cycle(PictureBus& bus);

    /// Perform the number of PPU cycles that fit into a clock cycle (3)
    inline void step(PictureBus& bus) { cycle(bus); cycle(bus); cycle(bus); };

    /// Reset the PPU
    void reset();

    /// Set the interrupt callback for the CPU
    void setInterruptCallback(std::function<void(void)> cb) { vblank_callback = cb; };

    void doDMA(const NES_Byte* page_ptr);

    //Callbacks mapped to CPU address space
    //Addresses written to by the program

    void control(NES_Byte ctrl);
    void setMask(NES_Byte mask);
    void setOAMAddress(NES_Byte address) { sprite_data_address = address; };
    void setDataAddress(NES_Byte address);
    void setScroll(NES_Byte scroll);
    void setData(PictureBus& bus, NES_Byte data);

    //Read by the program

    NES_Byte getStatus();
    NES_Byte getData(PictureBus& bus);
    NES_Byte getOAMData() { return read_OAM(sprite_data_address); };
    void setOAMData(NES_Byte value) { write_OAM(sprite_data_address++, value); };

    /// Return a pointer to the screen buffer.
    NES_Pixel* get_screen_buffer() { return *screen; };

};

#endif // PPU_HPP
