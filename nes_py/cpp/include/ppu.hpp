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

/// The number of cycles per scanline
const int SCANLINE_CYCLE_LENGTH = 341;
/// The last cycle of a scan line
const int SCANLINE_END_CYCLE = 340;
/// The number of visible scan lines (i.e., the height of the screen)
const int VISIBLE_SCANLINES = 240;
/// The number of visible dots per scan line (i.e., the width of the screen)
const int SCANLINE_VISIBLE_DOTS = 256;
/// The last memory frame of a scanline
const int FRAME_END_SCANLINE = 261;

/// The picture processing unit for the NES
class PPU {

private:
    std::function<void(void)> vblank_callback;

    std::vector<NES_Byte> sprite_memory;

    std::vector<NES_Byte> scanline_sprites;

    enum State {
        PreRender,
        Render,
        PostRender,
        VerticalBlank
    } pipeline_state;

    int m_cycles;
    int scanline;
    bool is_even_frame;

    bool vblank;
    bool sprite_zero_hit;

    // Registers
    NES_Address m_dataAddress;
    NES_Address m_tempAddress;
    NES_Byte m_fineXScroll;
    bool m_firstWrite;
    NES_Byte m_dataBuffer;

    NES_Byte m_spriteDataAddress;

    // Setup flags and variables
    bool m_longSprites;
    bool m_generateInterrupt;

    bool m_greyscaleMode;
    bool m_showSprites;
    bool m_showBackground;
    bool m_hideEdgeSprites;
    bool m_hideEdgeBackground;

    enum CharacterPage {
        Low,
        High,
    } m_bgPage, m_sprPage;

    NES_Address m_dataAddrIncrement;

    /// The internal screen data structure as a vector representation of a
    /// matrix of height matching the visible scans lines and width matching
    /// the number of visible scan line dots
    NES_Pixel screen[VISIBLE_SCANLINES][SCANLINE_VISIBLE_DOTS];

    NES_Byte readOAM(NES_Byte address) { return sprite_memory[address]; };
    void writeOAM(NES_Byte address, NES_Byte value) { sprite_memory[address] = value; };

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
    void setOAMAddress(NES_Byte address) { m_spriteDataAddress = address; };
    void setDataAddress(NES_Byte address);
    void setScroll(NES_Byte scroll);
    void setData(PictureBus& m_bus, NES_Byte data);
    //Read by the program
    NES_Byte getStatus();
    NES_Byte getData(PictureBus& m_bus);
    NES_Byte getOAMData() { return readOAM(m_spriteDataAddress); };
    void setOAMData(NES_Byte value) { writeOAM(m_spriteDataAddress++, value); };

    /// Return a pointer to the screen buffer.
    NES_Pixel* get_screen_buffer() { return *screen; };

};

#endif // PPU_HPP
