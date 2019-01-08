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
    longSprites = generateInterrupt = greyscaleMode = vblank = false;
    showBackground = showSprites = is_even_frame = firstWrite = true;
    bgPage = sprPage = Low;
    dataAddress = 0;
    cycles = 0;
    scanline = 0;
    spriteDataAddress = 0;
    fineXScroll = 0;
    tempAddress = 0;
    //baseNameTable = 0x2000;
    dataAddrIncrement = 1;
    pipeline_state = PreRender;
    scanline_sprites.reserve(8);
    scanline_sprites.resize(0);
}

void PPU::cycle(PictureBus& bus) {
    switch (pipeline_state) {
        case PreRender:
            if (cycles == 1)
                vblank = sprite_zero_hit = false;
            else if (cycles == SCANLINE_VISIBLE_DOTS + 2 && showBackground && showSprites) {
                //Set bits related to horizontal position
                dataAddress &= ~0x41f; //Unset horizontal bits
                dataAddress |= tempAddress & 0x41f; //Copy
            }
            else if (cycles > 280 && cycles <= 304 && showBackground && showSprites) {
                //Set vertical bits
                dataAddress &= ~0x7be0; //Unset bits related to horizontal
                dataAddress |= tempAddress & 0x7be0; //Copy
            }
            // if (cycles > 257 && cycles < 320)
            //     spriteDataAddress = 0;
            // if rendering is on, every other frame is one cycle shorter
            if (cycles >= SCANLINE_END_CYCLE - (!is_even_frame && showBackground && showSprites)) {
                pipeline_state = Render;
                cycles = scanline = 0;
            }
            break;
        case Render:
            if (cycles > 0 && cycles <= SCANLINE_VISIBLE_DOTS) {
                NES_Byte bgColor = 0, sprColor = 0;
                bool bgOpaque = false, sprOpaque = true;
                bool spriteForeground = false;

                int x = cycles - 1;
                int y = scanline;

                if (showBackground) {
                    auto x_fine = (fineXScroll + x) % 8;
                    if (!hideEdgeBackground || x >= 8) {
                        // fetch tile
                        // mask off fine y
                        auto address = 0x2000 | (dataAddress & 0x0FFF);
                        //auto address = 0x2000 + x / 8 + (y / 8) * (SCANLINE_VISIBLE_DOTS / 8);
                        NES_Byte tile = bus.read(address);

                        //fetch pattern
                        //Each pattern occupies 16 bytes, so multiply by 16
                        //Add fine y
                        address = (tile * 16) + ((dataAddress >> 12/*y % 8*/) & 0x7);
                        //set whether the pattern is in the high or low page
                        address |= bgPage << 12;
                        //Get the corresponding bit determined by (8 - x_fine) from the right
                        //bit 0 of palette entry
                        bgColor = (bus.read(address) >> (7 ^ x_fine)) & 1;
                        //bit 1
                        bgColor |= ((bus.read(address + 8) >> (7 ^ x_fine)) & 1) << 1;

                        //flag used to calculate final pixel with the sprite pixel
                        bgOpaque = bgColor;

                        //fetch attribute and calculate higher two bits of palette
                        address = 0x23C0 | (dataAddress & 0x0C00) | ((dataAddress >> 4) & 0x38)
                                    | ((dataAddress >> 2) & 0x07);
                        auto attribute = bus.read(address);
                        int shift = ((dataAddress >> 4) & 4) | (dataAddress & 2);
                        //Extract and set the upper two bits for the color
                        bgColor |= ((attribute >> shift) & 0x3) << 2;
                    }
                    //Increment/wrap coarse X
                    if (x_fine == 7) {
                        // if coarse X == 31
                        if ((dataAddress & 0x001F) == 31) {
                            // coarse X = 0
                            dataAddress &= ~0x001F;
                            // switch horizontal nametable
                            dataAddress ^= 0x0400;
                        }
                        else
                            // increment coarse X
                            dataAddress += 1;
                    }
                }

                if (showSprites && (!hideEdgeSprites || x >= 8)) {
                    for (auto i : scanline_sprites) {
                        NES_Byte spr_x =     sprite_memory[i * 4 + 3];

                        if (0 > x - spr_x || x - spr_x >= 8)
                            continue;

                        NES_Byte spr_y     = sprite_memory[i * 4 + 0] + 1,
                             tile      = sprite_memory[i * 4 + 1],
                             attribute = sprite_memory[i * 4 + 2];

                        int length = (longSprites) ? 16 : 8;

                        int x_shift = (x - spr_x) % 8, y_offset = (y - spr_y) % length;

                        if ((attribute & 0x40) == 0) //If NOT flipping horizontally
                            x_shift ^= 7;
                        if ((attribute & 0x80) != 0) //IF flipping vertically
                            y_offset ^= (length - 1);

                        NES_Address address = 0;

                        if (!longSprites) {
                            address = tile * 16 + y_offset;
                            if (sprPage == High) address += 0x1000;
                        }
                        // 8 x 16 sprites
                        else {
                            //bit-3 is one if it is the bottom tile of the sprite, multiply by two to get the next pattern
                            y_offset = (y_offset & 7) | ((y_offset & 8) << 1);
                            address = (tile >> 1) * 32 + y_offset;
                            address |= (tile & 1) << 12; //Bank 0x1000 if bit-0 is high
                        }

                        sprColor |= (bus.read(address) >> (x_shift)) & 1; //bit 0 of palette entry
                        sprColor |= ((bus.read(address + 8) >> (x_shift)) & 1) << 1; //bit 1

                        if (!(sprOpaque = sprColor)) {
                            sprColor = 0;
                            continue;
                        }

                        sprColor |= 0x10; //Select sprite palette
                        sprColor |= (attribute & 0x3) << 2; //bits 2-3

                        spriteForeground = !(attribute & 0x20);

                        //Sprite-0 hit detection
                        if (!sprite_zero_hit && showBackground && i == 0 && sprOpaque && bgOpaque)
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
                screen[y][x] = PALETTE[bus.read_palette(paletteAddr)];
            }
            else if (cycles == SCANLINE_VISIBLE_DOTS + 1 && showBackground) {
                //Shamelessly copied from nesdev wiki
                if ((dataAddress & 0x7000) != 0x7000)  // if fine Y < 7
                    dataAddress += 0x1000;              // increment fine Y
                else {
                    dataAddress &= ~0x7000;             // fine Y = 0
                    int y = (dataAddress & 0x03E0) >> 5;    // let y = coarse Y
                    if (y == 29) {
                        y = 0;                                // coarse Y = 0
                        dataAddress ^= 0x0800;              // switch vertical nametable
                    }
                    else if (y == 31)
                        y = 0;                                // coarse Y = 0, nametable not switched
                    else
                        y += 1;                               // increment coarse Y
                    dataAddress = (dataAddress & ~0x03E0) | (y << 5);
                                                            // put coarse Y back into dataAddress
                }
            }
            else if (cycles == SCANLINE_VISIBLE_DOTS + 2 && showBackground && showSprites) {
                //Copy bits related to horizontal position
                dataAddress &= ~0x41f;
                dataAddress |= tempAddress & 0x41f;
            }

//                 if (cycles > 257 && cycles < 320)
//                     spriteDataAddress = 0;

            if (cycles >= SCANLINE_END_CYCLE) {
                //Find and index sprites that are on the next Scanline
                //This isn't where/when this indexing, actually copying in 2C02 is done
                //but (I think) it shouldn't hurt any games if this is done here

                scanline_sprites.resize(0);

                int range = 8;
                if (longSprites)
                    range = 16;

                NES_Byte j = 0;
                for (NES_Byte i = spriteDataAddress / 4; i < 64; ++i) {
                    auto diff = (scanline - sprite_memory[i * 4]);
                    if (0 <= diff && diff < range) {
                        scanline_sprites.push_back(i);
                        if (++j >= 8)
                            break;
                    }
                }

                ++scanline;
                cycles = 0;
            }

            if (scanline >= VISIBLE_SCANLINES)
                pipeline_state = PostRender;

            break;
        case PostRender:
            if (cycles >= SCANLINE_END_CYCLE) {
                ++scanline;
                cycles = 0;
                pipeline_state = VerticalBlank;
            }

            break;
        case VerticalBlank:
            if (cycles == 1 && scanline == VISIBLE_SCANLINES + 1) {
                vblank = true;
                if (generateInterrupt) vblank_callback();
            }

            if (cycles >= SCANLINE_END_CYCLE) {
                ++scanline;
                cycles = 0;
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

    ++cycles;
}

void PPU::doDMA(const NES_Byte* page_ptr) {
    std::memcpy(sprite_memory.data() + spriteDataAddress, page_ptr, 256 - spriteDataAddress);
    if (spriteDataAddress)
        std::memcpy(sprite_memory.data(), page_ptr + (256 - spriteDataAddress), spriteDataAddress);
}

void PPU::control(NES_Byte ctrl) {
    generateInterrupt = ctrl & 0x80;
    longSprites = ctrl & 0x20;
    bgPage = static_cast<CharacterPage>(!!(ctrl & 0x10));
    sprPage = static_cast<CharacterPage>(!!(ctrl & 0x8));
    if (ctrl & 0x4)
        dataAddrIncrement = 0x20;
    else
        dataAddrIncrement = 1;
    //baseNameTable = (ctrl & 0x3) * 0x400 + 0x2000;

    //Set the nametable in the temp address, this will be reflected in the data address during rendering
    tempAddress &= ~0xc00;                 //Unset
    tempAddress |= (ctrl & 0x3) << 10;     //Set according to ctrl bits
}

void PPU::setMask(NES_Byte mask) {
    greyscaleMode = mask & 0x1;
    hideEdgeBackground = !(mask & 0x2);
    hideEdgeSprites = !(mask & 0x4);
    showBackground = mask & 0x8;
    showSprites = mask & 0x10;
}

NES_Byte PPU::getStatus() {
    NES_Byte status = sprite_zero_hit << 6 | vblank << 7;
    //dataAddress = 0;
    vblank = false;
    firstWrite = true;
    return status;
}

void PPU::setDataAddress(NES_Byte address) {
    //dataAddress = ((dataAddress << 8) & 0xff00) | address;
    if (firstWrite) {
        //Unset the upper byte
        tempAddress &= ~0xff00;
        tempAddress |= (address & 0x3f) << 8;
        firstWrite = false;
    }
    else {
        //Unset the lower byte;
        tempAddress &= ~0xff;
        tempAddress |= address;
        dataAddress = tempAddress;
        firstWrite = true;
    }
}

NES_Byte PPU::getData(PictureBus& bus) {
    auto data = bus.read(dataAddress);
    dataAddress += dataAddrIncrement;

    // Reads are delayed by one byte/read when address is in this range
    if (dataAddress < 0x3f00)
        //Return from the data buffer and store the current value in the buffer
        std::swap(data, dataBuffer);

    return data;
}

void PPU::setData(PictureBus& bus, NES_Byte data) {
    bus.write(dataAddress, data);
    dataAddress += dataAddrIncrement;
}

void PPU::setScroll(NES_Byte scroll) {
    if (firstWrite) {
        tempAddress &= ~0x1f;
        tempAddress |= (scroll >> 3) & 0x1f;
        fineXScroll = scroll & 0x7;
        firstWrite = false;
    }
    else {
        tempAddress &= ~0x73e0;
        tempAddress |= ((scroll & 0x7) << 12) |
                         ((scroll & 0xf8) << 2);
        firstWrite = true;
    }
}
