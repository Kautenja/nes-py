#pragma once
#include "common.hpp"

namespace PPU {
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

    template <bool write> u8 access(u16 index, u8 v = 0);
    void set_mirroring(Mirroring mode);
    void step();
    void reset();
}
