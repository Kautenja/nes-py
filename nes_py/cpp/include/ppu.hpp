//  Program:      nes-py
//  File:         ppu.hpp
//  Description:  This class houses the logic and data for the PPU of an NES
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef PPU_H
#define PPU_H

#include <functional>
#include <array>
#include "picture_bus.hpp"
#include "main_bus.hpp"
#include "palette_colors.hpp"

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
    uint8_t readOAM(uint8_t addr) { return m_spriteMemory[addr]; };
    void writeOAM(uint8_t addr, uint8_t value) { m_spriteMemory[addr] = value; };

    std::function<void(void)> m_vblankCallback;

    std::vector<uint8_t> m_spriteMemory;

    std::vector<uint8_t> m_scanlineSprites;

    enum State {
        PreRender,
        Render,
        PostRender,
        VerticalBlank
    } m_pipelineState;

    int m_cycle;
    int m_scanline;
    bool m_evenFrame;

    bool m_vblank;
    bool m_sprZeroHit;

    //Registers
    uint16_t m_dataAddress;
    uint16_t m_tempAddress;
    uint8_t m_fineXScroll;
    bool m_firstWrite;
    uint8_t m_dataBuffer;

    uint8_t m_spriteDataAddress;

    //Setup flags and variables
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

    uint16_t m_dataAddrIncrement;

    /// The internal screen data structure as a vector representation of a
    /// matrix of height matching the visible scans lines and width matching
    /// the number of visible scan line dots
    uint32_t screen_buffer[VISIBLE_SCANLINES][SCANLINE_VISIBLE_DOTS];

public:
    /// Initialize a new PPU
    PPU() : m_spriteMemory(64 * 4) { };

    /// Perform a single cycle on the PPU
    void cycle(PictureBus& bus);

    /// Perform the number of PPU cycles that fit into a clock cycle (3)
    inline void step(PictureBus& bus) { cycle(bus); cycle(bus); cycle(bus); };

    /// Reset the PPU
    void reset();

    /// Set the interrupt callback for the CPU
    void setInterruptCallback(std::function<void(void)> cb) { m_vblankCallback = cb; };

    void doDMA(const uint8_t* page_ptr);

    //Callbacks mapped to CPU address space
    //Addresses written to by the program
    void control(uint8_t ctrl);
    void setMask(uint8_t mask);
    void setOAMAddress(uint8_t addr) { m_spriteDataAddress = addr; };
    void setDataAddress(uint8_t addr);
    void setScroll(uint8_t scroll);
    void setData(PictureBus& m_bus, uint8_t data);
    //Read by the program
    uint8_t getStatus();
    uint8_t getData(PictureBus& m_bus);
    uint8_t getOAMData() { return readOAM(m_spriteDataAddress); };
    void setOAMData(uint8_t value) { writeOAM(m_spriteDataAddress++, value); };

    /// Return a pointer to the screen buffer.
    uint32_t* get_screen_buffer() { return *screen_buffer; };

};

#endif // PPU_H
