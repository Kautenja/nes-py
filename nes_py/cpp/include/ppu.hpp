#pragma once
#include <iostream>
#include "common.hpp"
#include "gui.hpp"
#include "cartridge.hpp"

/// Scanline configuration options
enum Scanline  { VISIBLE, POST, NMI, PRE };
/// Mirroring configuration options
enum Mirroring { VERTICAL, HORIZONTAL };

/// Sprite buffer
struct Sprite {
    /// Index in OAM
    u8 id;
    /// X position
    u8 x;
    /// Y position
    u8 y;
    /// Tile index
    u8 tile;
    /// Attributes
    u8 attr;
    /// Tile data (low)
    u8 dataL;
    /// Tile data (high)
    u8 dataH;
};

/// PPUCTRL ($2000) register
union Ctrl {
    struct {
        /// Nametable ($2000 / $2400 / $2800 / $2C00)
        unsigned nt     : 2;
        /// Address increment (1 / 32)
        unsigned incr   : 1;
        /// Sprite pattern table ($0000 / $1000)
        unsigned sprTbl : 1;
        /// BG pattern table ($0000 / $1000)
        unsigned bgTbl  : 1;
        /// Sprite size (8x8 / 8x16)
        unsigned sprSz  : 1;
        /// PPU master/slave
        unsigned slave  : 1;
        /// Enable NMI
        unsigned nmi    : 1;
    };
    u8 r;
};

/// PPUMASK ($2001) register
union Mask {
    struct {
        /// Grayscale
        unsigned gray    : 1;
        /// Show background in leftmost 8 pixels
        unsigned bgLeft  : 1;
        /// Show sprite in leftmost 8 pixels
        unsigned sprLeft : 1;
        /// Show background
        unsigned bg      : 1;
        /// Show sprites
        unsigned spr     : 1;
        /// Intensify reds
        unsigned red     : 1;
        /// Intensify greens
        unsigned green   : 1;
        /// Intensify blues
        unsigned blue    : 1;
    };
    u8 r;
};

/// PPUSTATUS ($2002) register
union Status {
    struct {
        /// Not significant
        unsigned bus    : 5;
        /// Sprite overflow
        unsigned sprOvf : 1;
        /// Sprite 0 Hit
        unsigned sprHit : 1;
        /// In VBlank?
        unsigned vBlank : 1;
    };
    u8 r;
};

/// Loopy's VRAM address
union Addr {
    struct {
        // Coarse X
        unsigned cX : 5;
        // Coarse Y
        unsigned cY : 5;
        // Nametable
        unsigned nt : 2;
        // Fine Y
        unsigned fY : 3;
    };
    struct {
        unsigned l : 8;
        unsigned h : 7;
    };
    unsigned addr : 14;
    unsigned r : 15;
};

/// A structure to contain all local variables of a PPU for state backup
struct PPUState {
    /// Mirroring mode
    Mirroring mirroring;
    /// VRAM for name-tables
    u8 ciRam[0x800];
    /// VRAM for palettes
    u8 cgRam[0x20];
    /// VRAM for sprite properties
    u8 oamMem[0x100];
    /// Sprite buffers
    Sprite oam[8], secOam[8];
    /// Video buffer
    u32 pixels[256 * 240];
    /// Loopy V, T
    Addr vAddr, tAddr;
    /// Fine X
    u8 fX;
    /// OAM address
    u8 oamAddr;
    /// PPUCTRL   ($2000) register
    Ctrl ctrl;
    /// PPUMASK   ($2001) register
    Mask mask;
    /// PPUSTATUS ($2002) register
    Status status;
    /// Background latches:
    u8 nt, at, bgL, bgH;
    /// Background shift registers:
    u8 atShiftL, atShiftH; u16 bgShiftL, bgShiftH;
    bool atLatchL, atLatchH;
    /// Rendering counters:
    int scanline, dot;
    bool frameOdd;

    /// Initialize a new PPU State
    PPUState() {
        frameOdd = false;
        scanline = dot = 0;
        ctrl.r = mask.r = status.r = 0;
        memset(pixels, 0x00, sizeof(pixels));
        memset(ciRam,  0xFF, sizeof(ciRam));
        memset(oamMem, 0x00, sizeof(oamMem));
    }

    /// Initialize a new PPU State as a copy of another
    PPUState(PPUState* state) {
        mirroring = state->mirroring;
        std::copy(std::begin(state->ciRam), std::end(state->ciRam), std::begin(ciRam));
        std::copy(std::begin(state->cgRam), std::end(state->cgRam), std::begin(cgRam));
        std::copy(std::begin(state->oamMem), std::end(state->oamMem), std::begin(oamMem));
        std::copy(std::begin(state->oam), std::end(state->oam), std::begin(oam));
        std::copy(std::begin(state->secOam), std::end(state->secOam), std::begin(secOam));
        std::copy(std::begin(state->pixels), std::end(state->pixels), std::begin(pixels));
        vAddr = state->vAddr;
        tAddr = state->tAddr;
        fX = state->fX;
        oamAddr = state->oamAddr;
        ctrl = state->ctrl;
        mask = state->mask;
        status = state->status;
        nt = state->nt;
        at = state->at;
        bgL = state->bgL;
        bgH = state->bgH;
        atShiftL = state->atShiftL;
        atShiftH = state->atShiftH;
        bgShiftL = state->bgShiftL;
        bgShiftH = state->bgShiftH;
        atLatchL = state->atLatchL;
        atLatchH = state->atLatchH;
        scanline = state->scanline;
        dot = state->dot;
        frameOdd = state->frameOdd;
    }
};

/// The Picture Processing Unit
namespace PPU {

    /// Set the GUI instance pointer to a new value.
    void set_gui(GUI* new_gui);

    /// Return the pointer to this PPU's GUI instance
    GUI* get_gui();

    /// Set the Cartridge instance pointer to a new value.
    void set_cartridge(Cartridge* new_cartridge);

    /// Return the pointer to this PPU's Cartridge instance
    Cartridge* get_cartridge();

    template <bool write> u8 access(u16 index, u8 v = 0);
    void set_mirroring(Mirroring mode);

    /// Execute a PPU cycle.
    void step();

    /// Reset the PPU to a blank state.
    void reset();

    /// Return a new PPU state of the PPU variables
    PPUState* get_state();

    /// Restore the PPU variables from a saved state
    void set_state(PPUState* state);
}
