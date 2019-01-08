//  Program:      nes-py
//  File:         ppu.cpp
//  Description:  This class houses the logic and data for the PPU of an NES
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "ppu.hpp"
#include "palette.hpp"
#include "log.hpp"

void PPU::reset() {
    m_longSprites = m_generateInterrupt = m_greyscaleMode = vblank = false;
    m_showBackground = m_showSprites = is_even_frame = m_firstWrite = true;
    m_bgPage = m_sprPage = Low;
    m_dataAddress = 0;
    m_cycles = 0;
    scanline = 0;
    m_spriteDataAddress = 0;
    m_fineXScroll = 0;
    m_tempAddress = 0;
    //m_baseNameTable = 0x2000;
    m_dataAddrIncrement = 1;
    pipeline_state = PreRender;
    scanline_sprites.reserve(8);
    scanline_sprites.resize(0);
}

void PPU::cycle(PictureBus& m_bus) {
    switch (pipeline_state) {
        case PreRender:
            if (m_cycles == 1)
                vblank = sprite_zero_hit = false;
            else if (m_cycles == SCANLINE_VISIBLE_DOTS + 2 && m_showBackground && m_showSprites) {
                //Set bits related to horizontal position
                m_dataAddress &= ~0x41f; //Unset horizontal bits
                m_dataAddress |= m_tempAddress & 0x41f; //Copy
            }
            else if (m_cycles > 280 && m_cycles <= 304 && m_showBackground && m_showSprites) {
                //Set vertical bits
                m_dataAddress &= ~0x7be0; //Unset bits related to horizontal
                m_dataAddress |= m_tempAddress & 0x7be0; //Copy
            }
            // if (m_cycles > 257 && m_cycles < 320)
            //     m_spriteDataAddress = 0;
            // if rendering is on, every other frame is one cycle shorter
            if (m_cycles >= SCANLINE_END_CYCLE - (!is_even_frame && m_showBackground && m_showSprites)) {
                pipeline_state = Render;
                m_cycles = scanline = 0;
            }
            break;
        case Render:
            if (m_cycles > 0 && m_cycles <= SCANLINE_VISIBLE_DOTS) {
                NES_Byte bgColor = 0, sprColor = 0;
                bool bgOpaque = false, sprOpaque = true;
                bool spriteForeground = false;

                int x = m_cycles - 1;
                int y = scanline;

                if (m_showBackground) {
                    auto x_fine = (m_fineXScroll + x) % 8;
                    if (!m_hideEdgeBackground || x >= 8) {
                        // fetch tile
                        // mask off fine y
                        auto address = 0x2000 | (m_dataAddress & 0x0FFF);
                        //auto address = 0x2000 + x / 8 + (y / 8) * (SCANLINE_VISIBLE_DOTS / 8);
                        NES_Byte tile = m_bus.read(address);

                        //fetch pattern
                        //Each pattern occupies 16 bytes, so multiply by 16
                        //Add fine y
                        address = (tile * 16) + ((m_dataAddress >> 12/*y % 8*/) & 0x7);
                        //set whether the pattern is in the high or low page
                        address |= m_bgPage << 12;
                        //Get the corresponding bit determined by (8 - x_fine) from the right
                        //bit 0 of palette entry
                        bgColor = (m_bus.read(address) >> (7 ^ x_fine)) & 1;
                        //bit 1
                        bgColor |= ((m_bus.read(address + 8) >> (7 ^ x_fine)) & 1) << 1;

                        //flag used to calculate final pixel with the sprite pixel
                        bgOpaque = bgColor;

                        //fetch attribute and calculate higher two bits of palette
                        address = 0x23C0 | (m_dataAddress & 0x0C00) | ((m_dataAddress >> 4) & 0x38)
                                    | ((m_dataAddress >> 2) & 0x07);
                        auto attribute = m_bus.read(address);
                        int shift = ((m_dataAddress >> 4) & 4) | (m_dataAddress & 2);
                        //Extract and set the upper two bits for the color
                        bgColor |= ((attribute >> shift) & 0x3) << 2;
                    }
                    //Increment/wrap coarse X
                    if (x_fine == 7) {
                        // if coarse X == 31
                        if ((m_dataAddress & 0x001F) == 31) {
                            // coarse X = 0
                            m_dataAddress &= ~0x001F;
                            // switch horizontal nametable
                            m_dataAddress ^= 0x0400;
                        }
                        else
                            // increment coarse X
                            m_dataAddress += 1;
                    }
                }

                if (m_showSprites && (!m_hideEdgeSprites || x >= 8)) {
                    for (auto i : scanline_sprites) {
                        NES_Byte spr_x =     sprite_memory[i * 4 + 3];

                        if (0 > x - spr_x || x - spr_x >= 8)
                            continue;

                        NES_Byte spr_y     = sprite_memory[i * 4 + 0] + 1,
                             tile      = sprite_memory[i * 4 + 1],
                             attribute = sprite_memory[i * 4 + 2];

                        int length = (m_longSprites) ? 16 : 8;

                        int x_shift = (x - spr_x) % 8, y_offset = (y - spr_y) % length;

                        if ((attribute & 0x40) == 0) //If NOT flipping horizontally
                            x_shift ^= 7;
                        if ((attribute & 0x80) != 0) //IF flipping vertically
                            y_offset ^= (length - 1);

                        NES_Address address = 0;

                        if (!m_longSprites) {
                            address = tile * 16 + y_offset;
                            if (m_sprPage == High) address += 0x1000;
                        }
                        // 8 x 16 sprites
                        else {
                            //bit-3 is one if it is the bottom tile of the sprite, multiply by two to get the next pattern
                            y_offset = (y_offset & 7) | ((y_offset & 8) << 1);
                            address = (tile >> 1) * 32 + y_offset;
                            address |= (tile & 1) << 12; //Bank 0x1000 if bit-0 is high
                        }

                        sprColor |= (m_bus.read(address) >> (x_shift)) & 1; //bit 0 of palette entry
                        sprColor |= ((m_bus.read(address + 8) >> (x_shift)) & 1) << 1; //bit 1

                        if (!(sprOpaque = sprColor)) {
                            sprColor = 0;
                            continue;
                        }

                        sprColor |= 0x10; //Select sprite palette
                        sprColor |= (attribute & 0x3) << 2; //bits 2-3

                        spriteForeground = !(attribute & 0x20);

                        //Sprite-0 hit detection
                        if (!sprite_zero_hit && m_showBackground && i == 0 && sprOpaque && bgOpaque)
                            sprite_zero_hit = true;

                        break; //Exit the loop now since we've found the highest priority sprite
                    }
                }

                NES_Byte paletteAddr = bgColor;

                if ( (!bgOpaque && sprOpaque) || (bgOpaque && sprOpaque && spriteForeground) )
                    paletteAddr = sprColor;
                else if (!bgOpaque && !sprOpaque)
                    paletteAddr = 0;
                // lookup the pixel in the palette and write it to the screen
                screen[y][x] = PALETTE[m_bus.read_palette(paletteAddr)];
            }
            else if (m_cycles == SCANLINE_VISIBLE_DOTS + 1 && m_showBackground) {
                //Shamelessly copied from nesdev wiki
                if ((m_dataAddress & 0x7000) != 0x7000)  // if fine Y < 7
                    m_dataAddress += 0x1000;              // increment fine Y
                else {
                    m_dataAddress &= ~0x7000;             // fine Y = 0
                    int y = (m_dataAddress & 0x03E0) >> 5;    // let y = coarse Y
                    if (y == 29) {
                        y = 0;                                // coarse Y = 0
                        m_dataAddress ^= 0x0800;              // switch vertical nametable
                    }
                    else if (y == 31)
                        y = 0;                                // coarse Y = 0, nametable not switched
                    else
                        y += 1;                               // increment coarse Y
                    m_dataAddress = (m_dataAddress & ~0x03E0) | (y << 5);
                                                            // put coarse Y back into m_dataAddress
                }
            }
            else if (m_cycles == SCANLINE_VISIBLE_DOTS + 2 && m_showBackground && m_showSprites) {
                //Copy bits related to horizontal position
                m_dataAddress &= ~0x41f;
                m_dataAddress |= m_tempAddress & 0x41f;
            }

//                 if (m_cycles > 257 && m_cycles < 320)
//                     m_spriteDataAddress = 0;

            if (m_cycles >= SCANLINE_END_CYCLE) {
                //Find and index sprites that are on the next Scanline
                //This isn't where/when this indexing, actually copying in 2C02 is done
                //but (I think) it shouldn't hurt any games if this is done here

                scanline_sprites.resize(0);

                int range = 8;
                if (m_longSprites)
                    range = 16;

                NES_Byte j = 0;
                for (NES_Byte i = m_spriteDataAddress / 4; i < 64; ++i) {
                    auto diff = (scanline - sprite_memory[i * 4]);
                    if (0 <= diff && diff < range) {
                        scanline_sprites.push_back(i);
                        if (++j >= 8)
                            break;
                    }
                }

                ++scanline;
                m_cycles = 0;
            }

            if (scanline >= VISIBLE_SCANLINES)
                pipeline_state = PostRender;

            break;
        case PostRender:
            if (m_cycles >= SCANLINE_END_CYCLE) {
                ++scanline;
                m_cycles = 0;
                pipeline_state = VerticalBlank;
            }

            break;
        case VerticalBlank:
            if (m_cycles == 1 && scanline == VISIBLE_SCANLINES + 1) {
                vblank = true;
                if (m_generateInterrupt) vblank_callback();
            }

            if (m_cycles >= SCANLINE_END_CYCLE) {
                ++scanline;
                m_cycles = 0;
            }

            if (scanline >= FRAME_END_SCANLINE) {
                pipeline_state = PreRender;
                scanline = 0;
                is_even_frame = !is_even_frame;
                // vblank = false;
            }

            break;
        default:
            LOG(Error) << "Well, this shouldn't have happened." << std::endl;
    }

    ++m_cycles;
}

void PPU::doDMA(const NES_Byte* page_ptr) {
    std::memcpy(sprite_memory.data() + m_spriteDataAddress, page_ptr, 256 - m_spriteDataAddress);
    if (m_spriteDataAddress)
        std::memcpy(sprite_memory.data(), page_ptr + (256 - m_spriteDataAddress), m_spriteDataAddress);
}

void PPU::control(NES_Byte ctrl) {
    m_generateInterrupt = ctrl & 0x80;
    m_longSprites = ctrl & 0x20;
    m_bgPage = static_cast<CharacterPage>(!!(ctrl & 0x10));
    m_sprPage = static_cast<CharacterPage>(!!(ctrl & 0x8));
    if (ctrl & 0x4)
        m_dataAddrIncrement = 0x20;
    else
        m_dataAddrIncrement = 1;
    //m_baseNameTable = (ctrl & 0x3) * 0x400 + 0x2000;

    //Set the nametable in the temp address, this will be reflected in the data address during rendering
    m_tempAddress &= ~0xc00;                 //Unset
    m_tempAddress |= (ctrl & 0x3) << 10;     //Set according to ctrl bits
}

void PPU::setMask(NES_Byte mask) {
    m_greyscaleMode = mask & 0x1;
    m_hideEdgeBackground = !(mask & 0x2);
    m_hideEdgeSprites = !(mask & 0x4);
    m_showBackground = mask & 0x8;
    m_showSprites = mask & 0x10;
}

NES_Byte PPU::getStatus() {
    NES_Byte status = sprite_zero_hit << 6 | vblank << 7;
    //m_dataAddress = 0;
    vblank = false;
    m_firstWrite = true;
    return status;
}

void PPU::setDataAddress(NES_Byte address) {
    //m_dataAddress = ((m_dataAddress << 8) & 0xff00) | address;
    if (m_firstWrite) {
        //Unset the upper byte
        m_tempAddress &= ~0xff00;
        m_tempAddress |= (address & 0x3f) << 8;
        m_firstWrite = false;
    }
    else {
        //Unset the lower byte;
        m_tempAddress &= ~0xff;
        m_tempAddress |= address;
        m_dataAddress = m_tempAddress;
        m_firstWrite = true;
    }
}

NES_Byte PPU::getData(PictureBus& m_bus) {
    auto data = m_bus.read(m_dataAddress);
    m_dataAddress += m_dataAddrIncrement;

    // Reads are delayed by one byte/read when address is in this range
    if (m_dataAddress < 0x3f00)
        //Return from the data buffer and store the current value in the buffer
        std::swap(data, m_dataBuffer);

    return data;
}

void PPU::setData(PictureBus& m_bus, NES_Byte data) {
    m_bus.write(m_dataAddress, data);
    m_dataAddress += m_dataAddrIncrement;
}

void PPU::setScroll(NES_Byte scroll) {
    if (m_firstWrite) {
        m_tempAddress &= ~0x1f;
        m_tempAddress |= (scroll >> 3) & 0x1f;
        m_fineXScroll = scroll & 0x7;
        m_firstWrite = false;
    }
    else {
        m_tempAddress &= ~0x73e0;
        m_tempAddress |= ((scroll & 0x7) << 12) |
                         ((scroll & 0xf8) << 2);
        m_firstWrite = true;
    }
}
