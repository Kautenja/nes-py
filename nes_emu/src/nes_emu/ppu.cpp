//  Program:      nes-py
//  File:         ppu.cpp
//  Description:  This class houses the logic and data for the PPU of an NES
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include <algorithm>
#include <cstring>
#include "nes_emu/ppu.hpp"
#include "nes_emu/palette.hpp"
#include "nes_emu/log.hpp"

namespace NES {

void PPU::reset() {
    is_long_sprites = false;
    is_interrupting = false;
    is_vblank = false;
    is_sprite_zero_hit = false;
    is_showing_background = true;
    is_showing_sprites = true;
    is_hiding_edge_background = false;
    is_hiding_edge_sprites = false;
    is_even_frame = true;
    is_first_write = true;
    background_page = LOW;
    sprite_page = LOW;
    data_address = 0;
    cycles = 0;
    scanline = 0;
    sprite_data_address = 0;
    fine_x_scroll = 0;
    temp_address = 0;
    data_buffer = 0;
    data_address_increment = 1;
    pipeline_state = PRE_RENDER;
    scanline_sprite_count = 0;
    scanline_sprite_rows_cached = false;
    scanline_sprite_rows_generation = 0;
    background_tile_cache_valid = false;
    background_tile_cache_address = 0;
    background_tile_cache_page = LOW;
    background_tile_cache_generation = 0;
    background_tile_cache_low = 0;
    background_tile_cache_high = 0;
    background_tile_cache_attribute = 0;
    std::fill(sprite_memory.begin(), sprite_memory.end(), 0);
    std::fill(scanline_sprites.begin(), scanline_sprites.end(), 0);
    std::fill(
        scanline_sprite_rows.begin(),
        scanline_sprite_rows.end(),
        SpriteRow()
    );
    std::fill(&screen[0][0], &screen[0][0] + SCREEN_PIXEL_COUNT, 0);
}

PPU::Snapshot PPU::save_state() const {
    Snapshot snapshot;
    snapshot.sprite_memory = sprite_memory;
    snapshot.scanline_sprites = scanline_sprites;
    snapshot.scanline_sprite_count = scanline_sprite_count;
    snapshot.scanline_sprite_rows = scanline_sprite_rows;
    snapshot.scanline_sprite_rows_cached = scanline_sprite_rows_cached;
    snapshot.scanline_sprite_rows_generation = scanline_sprite_rows_generation;
    snapshot.pipeline_state = pipeline_state;
    snapshot.cycles = cycles;
    snapshot.scanline = scanline;
    snapshot.is_even_frame = is_even_frame;
    snapshot.is_vblank = is_vblank;
    snapshot.is_sprite_zero_hit = is_sprite_zero_hit;
    snapshot.data_address = data_address;
    snapshot.temp_address = temp_address;
    snapshot.fine_x_scroll = fine_x_scroll;
    snapshot.is_first_write = is_first_write;
    snapshot.data_buffer = data_buffer;
    snapshot.sprite_data_address = sprite_data_address;
    snapshot.is_showing_sprites = is_showing_sprites;
    snapshot.is_showing_background = is_showing_background;
    snapshot.is_hiding_edge_sprites = is_hiding_edge_sprites;
    snapshot.is_hiding_edge_background = is_hiding_edge_background;
    snapshot.is_long_sprites = is_long_sprites;
    snapshot.is_interrupting = is_interrupting;
    snapshot.background_page = background_page;
    snapshot.sprite_page = sprite_page;
    snapshot.data_address_increment = data_address_increment;
    snapshot.background_tile_cache_valid = background_tile_cache_valid;
    snapshot.background_tile_cache_address = background_tile_cache_address;
    snapshot.background_tile_cache_page = background_tile_cache_page;
    snapshot.background_tile_cache_generation = background_tile_cache_generation;
    snapshot.background_tile_cache_low = background_tile_cache_low;
    snapshot.background_tile_cache_high = background_tile_cache_high;
    snapshot.background_tile_cache_attribute = background_tile_cache_attribute;
    std::copy(
        &screen[0][0],
        &screen[0][0] + SCREEN_PIXEL_COUNT,
        snapshot.screen.begin()
    );
    return snapshot;
}

void PPU::load_state(const Snapshot& snapshot) {
    sprite_memory = snapshot.sprite_memory;
    scanline_sprites = snapshot.scanline_sprites;
    scanline_sprite_count = snapshot.scanline_sprite_count;
    scanline_sprite_rows = snapshot.scanline_sprite_rows;
    scanline_sprite_rows_cached = snapshot.scanline_sprite_rows_cached;
    scanline_sprite_rows_generation = snapshot.scanline_sprite_rows_generation;
    pipeline_state = static_cast<PipelineState>(snapshot.pipeline_state);
    cycles = snapshot.cycles;
    scanline = snapshot.scanline;
    is_even_frame = snapshot.is_even_frame;
    is_vblank = snapshot.is_vblank;
    is_sprite_zero_hit = snapshot.is_sprite_zero_hit;
    data_address = snapshot.data_address;
    temp_address = snapshot.temp_address;
    fine_x_scroll = snapshot.fine_x_scroll;
    is_first_write = snapshot.is_first_write;
    data_buffer = snapshot.data_buffer;
    sprite_data_address = snapshot.sprite_data_address;
    is_showing_sprites = snapshot.is_showing_sprites;
    is_showing_background = snapshot.is_showing_background;
    is_hiding_edge_sprites = snapshot.is_hiding_edge_sprites;
    is_hiding_edge_background = snapshot.is_hiding_edge_background;
    is_long_sprites = snapshot.is_long_sprites;
    is_interrupting = snapshot.is_interrupting;
    background_page = static_cast<CharacterPage>(snapshot.background_page);
    sprite_page = static_cast<CharacterPage>(snapshot.sprite_page);
    data_address_increment = snapshot.data_address_increment;
    background_tile_cache_valid = snapshot.background_tile_cache_valid;
    background_tile_cache_address = snapshot.background_tile_cache_address;
    background_tile_cache_page = static_cast<CharacterPage>(
        snapshot.background_tile_cache_page
    );
    background_tile_cache_generation =
        snapshot.background_tile_cache_generation;
    background_tile_cache_low = snapshot.background_tile_cache_low;
    background_tile_cache_high = snapshot.background_tile_cache_high;
    background_tile_cache_attribute = snapshot.background_tile_cache_attribute;
    std::copy(snapshot.screen.begin(), snapshot.screen.end(), &screen[0][0]);
}

void PPU::invalidate_sprite_rows() {
    scanline_sprite_rows_cached = false;
}

NES_Address PPU::sprite_pattern_address(NES_Byte tile, int y_offset) const {
    NES_Address address = 0;
    if (!is_long_sprites) {
        address = tile * 16 + y_offset;
        if (sprite_page == HIGH)
            address += 0x1000;
    } else {
        y_offset = (y_offset & 7) | ((y_offset & 8) << 1);
        address = (tile >> 1) * 32 + y_offset;
        address |= (tile & 1) << 12;
    }
    return address;
}

void PPU::prefetch_scanline_sprite_rows(PictureBus& bus) {
    std::fill(
        scanline_sprite_rows.begin(),
        scanline_sprite_rows.end(),
        SpriteRow()
    );
    scanline_sprite_rows_generation = bus.get_write_generation();

    const int length = is_long_sprites ? 16 : 8;
    for (std::size_t row_index = 0; row_index < scanline_sprite_count;
            ++row_index) {
        const NES_Byte sprite_index = scanline_sprites[row_index];
        const std::size_t oam_index = sprite_index * 4;
        NES_Byte tile = sprite_memory[oam_index + 1];
        NES_Byte attribute = sprite_memory[oam_index + 2];
        int y_offset = scanline - sprite_memory[oam_index];
        if ((attribute & 0x80) != 0)
            y_offset ^= (length - 1);

        const NES_Address address = sprite_pattern_address(tile, y_offset);
        SpriteRow& row = scanline_sprite_rows[row_index];
        row.sprite_index = sprite_index;
        row.x = sprite_memory[oam_index + 3];
        row.attribute = attribute;
        row.pattern_low = bus.read(address);
        row.pattern_high = bus.read(address + 8);
        row.palette_base = 0x10 | ((attribute & 0x3) << 2);
        row.foreground = !(attribute & 0x20);
        for (int pixel = 0; pixel < 8; ++pixel) {
            int x_shift = pixel;
            if ((attribute & 0x40) == 0)
                x_shift ^= 7;
            row.pixels[pixel] = (row.pattern_low >> x_shift) & 1;
            row.pixels[pixel] |= (
                (row.pattern_high >> x_shift) & 1
            ) << 1;
        }
        row.valid = true;
    }
    scanline_sprite_rows_cached = true;
}

void PPU::evaluate_scanline_sprites(PictureBus& bus) {
    scanline_sprite_count = 0;
    std::fill(scanline_sprites.begin(), scanline_sprites.end(), 0);
    invalidate_sprite_rows();

    const int range = is_long_sprites ? 16 : 8;
    for (NES_Byte i = sprite_data_address / 4; i < 64; ++i) {
        auto diff = (scanline - sprite_memory[i * 4]);
        if (0 <= diff && diff < range) {
            scanline_sprites[scanline_sprite_count++] = i;
            if (scanline_sprite_count >= MAX_SCANLINE_SPRITES)
                break;
        }
    }

    if (
        is_showing_sprites &&
        scanline_sprite_count >= SPRITE_PREFETCH_MIN_SPRITES &&
        bus.can_prefetch_sprite_rows()
    )
        prefetch_scanline_sprite_rows(bus);
}

void PPU::cycle(PictureBus& bus) {
    switch (pipeline_state) {
        case PRE_RENDER: {
            if (cycles == 1)
                is_vblank = is_sprite_zero_hit = false;
            else if (cycles == SCANLINE_VISIBLE_DOTS + 2 && is_showing_background && is_showing_sprites) {
                // Set bits related to horizontal position
                data_address &= ~0x41f; //Unset horizontal bits
                data_address |= temp_address & 0x41f; //Copy
                background_tile_cache_valid = false;
            }
            else if (cycles > 280 && cycles <= 304 && is_showing_background && is_showing_sprites) {
                // Set vertical bits
                data_address &= ~0x7be0; //Unset bits related to horizontal
                data_address |= temp_address & 0x7be0; //Copy
                background_tile_cache_valid = false;
            }
            // if (cycles > 257 && cycles < 320)
            //     sprite_data_address = 0;
            // if rendering is on, every other frame is one cycle shorter
            if (cycles >= SCANLINE_END_CYCLE - (!is_even_frame && is_showing_background && is_showing_sprites)) {
                pipeline_state = RENDER;
                cycles = scanline = 0;
            }
            break;
        }
        case RENDER: {
            if (cycles > 0 && cycles <= SCANLINE_VISIBLE_DOTS) {
                NES_Byte bgColor = 0, sprColor = 0;
                bool bgOpaque = false, sprOpaque = false;
                bool spriteForeground = false;

                int x = cycles - 1;
                int y = scanline;

                if (is_showing_background) {
                    auto x_fine = (fine_x_scroll + x) % 8;
                    if (!is_hiding_edge_background || x >= 8) {
                        NES_Byte pattern_low = 0;
                        NES_Byte pattern_high = 0;
                        NES_Byte attribute = 0;
                        bool can_cache = !bus.has_mapper_ppu_observers();
                        auto generation = bus.get_write_generation();

                        if (
                            can_cache &&
                            background_tile_cache_valid &&
                            background_tile_cache_address == data_address &&
                            background_tile_cache_page == background_page &&
                            background_tile_cache_generation == generation
                        ) {
                            pattern_low = background_tile_cache_low;
                            pattern_high = background_tile_cache_high;
                            attribute = background_tile_cache_attribute;
                        } else {
                            // fetch tile
                            // mask off fine y
                            auto address = 0x2000 | (data_address & 0x0FFF);
                            NES_Byte tile = bus.read(address);

                            //fetch pattern
                            //Each pattern occupies 16 bytes, so multiply by 16
                            //Add fine y
                            address = (tile * 16) + ((data_address >> 12/*y % 8*/) & 0x7);
                            //set whether the pattern is in the high or low page
                            address |= background_page << 12;
                            pattern_low = bus.read(address);
                            pattern_high = bus.read(address + 8);

                            //fetch attribute and calculate higher two bits of palette
                            address = 0x23C0 | (data_address & 0x0C00) | ((data_address >> 4) & 0x38)
                                        | ((data_address >> 2) & 0x07);
                            attribute = bus.read(address);

                            if (can_cache) {
                                background_tile_cache_valid = true;
                                background_tile_cache_address = data_address;
                                background_tile_cache_page = background_page;
                                background_tile_cache_generation = generation;
                                background_tile_cache_low = pattern_low;
                                background_tile_cache_high = pattern_high;
                                background_tile_cache_attribute = attribute;
                            } else {
                                background_tile_cache_valid = false;
                            }
                        }

                        //Get the corresponding bit determined by (8 - x_fine) from the right
                        //bit 0 of palette entry
                        bgColor = (pattern_low >> (7 ^ x_fine)) & 1;
                        //bit 1
                        bgColor |= ((pattern_high >> (7 ^ x_fine)) & 1) << 1;

                        //flag used to calculate final pixel with the sprite pixel
                        bgOpaque = bgColor;

                        int shift = ((data_address >> 4) & 4) | (data_address & 2);
                        //Extract and set the upper two bits for the color
                        bgColor |= ((attribute >> shift) & 0x3) << 2;
                    }
                    //Increment/wrap coarse X
                    if (x_fine == 7) {
                        // if coarse X == 31
                        if ((data_address & 0x001F) == 31) {
                            // coarse X = 0
                            data_address &= ~0x001F;
                            // switch horizontal nametable
                            data_address ^= 0x0400;
                        }
                        else
                            // increment coarse X
                            data_address += 1;
                        background_tile_cache_valid = false;
                    }
                }

                if (is_showing_sprites && (!is_hiding_edge_sprites || x >= 8)) {
                    bool use_cached_rows = scanline_sprite_rows_cached;
                    if (
                        use_cached_rows &&
                        scanline_sprite_rows_generation !=
                            bus.get_write_generation()
                    ) {
                        invalidate_sprite_rows();
                        use_cached_rows = false;
                    }

                    if (use_cached_rows) {
                        for (std::size_t row_index = 0;
                                row_index < scanline_sprite_count;
                                ++row_index) {
                            const SpriteRow& row =
                                scanline_sprite_rows[row_index];
                            if (!row.valid)
                                continue;

                            if (0 > x - row.x || x - row.x >= 8)
                                continue;

                            NES_Byte candidate = row.pixels[x - row.x];

                            if (!(sprOpaque = candidate)) {
                                sprColor = 0;
                                continue;
                            }

                            sprColor = row.palette_base | candidate;
                            spriteForeground = row.foreground;

                            if (
                                !is_sprite_zero_hit &&
                                is_showing_background &&
                                row.sprite_index == 0 &&
                                sprOpaque &&
                                bgOpaque
                            )
                                is_sprite_zero_hit = true;

                            break;
                        }
                    } else {
                        for (std::size_t sprite_index = 0; sprite_index < scanline_sprite_count; ++sprite_index) {
                            NES_Byte i = scanline_sprites[sprite_index];
                            NES_Byte spr_x =     sprite_memory[i * 4 + 3];

                            if (0 > x - spr_x || x - spr_x >= 8)
                                continue;

                            NES_Byte spr_y     = sprite_memory[i * 4 + 0] + 1,
                                 tile      = sprite_memory[i * 4 + 1],
                                 attribute = sprite_memory[i * 4 + 2];

                            int length = (is_long_sprites) ? 16 : 8;

                            int x_shift = (x - spr_x) % 8, y_offset = (y - spr_y) % length;

                            if ((attribute & 0x40) == 0) //If NOT flipping horizontally
                                x_shift ^= 7;
                            if ((attribute & 0x80) != 0) //IF flipping vertically
                                y_offset ^= (length - 1);

                            NES_Address address =
                                sprite_pattern_address(tile, y_offset);

                            sprColor = (bus.read(address) >> (x_shift)) & 1; //bit 0 of palette entry
                            sprColor |= ((bus.read(address + 8) >> (x_shift)) & 1) << 1; //bit 1

                            if (!(sprOpaque = sprColor)) {
                                sprColor = 0;
                                continue;
                            }

                            sprColor |= 0x10; //Select sprite palette
                            sprColor |= (attribute & 0x3) << 2; //bits 2-3

                            spriteForeground = !(attribute & 0x20);

                            //Sprite-0 hit detection
                            if (!is_sprite_zero_hit && is_showing_background && i == 0 && sprOpaque && bgOpaque)
                                is_sprite_zero_hit = true;

                            break; //Exit the loop now since we've found the highest priority sprite
                        }
                    }
                }
                // get the address of the color in the palette
                NES_Byte paletteAddr = bgColor;
                if ( (!bgOpaque && sprOpaque) || (bgOpaque && sprOpaque && spriteForeground) )
                    paletteAddr = sprColor;
                else if (!bgOpaque && !sprOpaque)
                    paletteAddr = 0;
                // lookup the pixel in the palette and write it to the screen
                screen[y][x] = PALETTE[bus.read_palette(paletteAddr)];
            }
            else if (cycles == SCANLINE_VISIBLE_DOTS + 1 && is_showing_background) {
                //Shamelessly copied from nesdev wiki
                if ((data_address & 0x7000) != 0x7000) {  // if fine Y < 7
                    // increment fine Y
                    data_address += 0x1000;
                } else {
                    // fine Y = 0
                    data_address &= ~0x7000;
                    // let y = coarse Y
                    int y = (data_address & 0x03E0) >> 5;
                    if (y == 29) {
                        // coarse Y = 0
                        y = 0;
                        // switch vertical nametable
                        data_address ^= 0x0800;
                    } else if (y == 31) {
                        // coarse Y = 0, nametable not switched
                        y = 0;
                    } else {
                        // increment coarse Y
                        y += 1;
                    }
                    // put coarse Y back into data_address
                    data_address = (data_address & ~0x03E0) | (y << 5);
                }
            }
            else if (cycles == SCANLINE_VISIBLE_DOTS + 2 && is_showing_background && is_showing_sprites) {
                // Copy bits related to horizontal position
                data_address &= ~0x41f;
                data_address |= temp_address & 0x41f;
                background_tile_cache_valid = false;
            }

//                 if (cycles > 257 && cycles < 320)
//                     sprite_data_address = 0;

            if (cycles >= SCANLINE_END_CYCLE) {
                evaluate_scanline_sprites(bus);
                background_tile_cache_valid = false;

                ++scanline;
                cycles = 0;
            }

            if (scanline >= VISIBLE_SCANLINES)
                pipeline_state = POST_RENDER;

            break;
        }
        case POST_RENDER: {
            if (cycles >= SCANLINE_END_CYCLE) {
                ++scanline;
                cycles = 0;
                pipeline_state = VERTICAL_BLANK;
            }
            break;
        }
        case VERTICAL_BLANK: {
            if (cycles == 1 && scanline == VISIBLE_SCANLINES + 1) {
                is_vblank = true;
                if (is_interrupting) vblank_callback();
            }

            if (cycles >= SCANLINE_END_CYCLE) {
                ++scanline;
                cycles = 0;
            }

            if (scanline >= FRAME_END_SCANLINE) {
                pipeline_state = PRE_RENDER;
                scanline = 0;
                is_even_frame = !is_even_frame;
                // is_vblank = false;
            }

            break;
        }
        default:
            LOG(Error) << "Well, this shouldn't have happened." << std::endl;
    }
    ++cycles;
}

void PPU::do_DMA(const NES_Byte* page_ptr) {
    std::memcpy(
        sprite_memory.data() + sprite_data_address,
        page_ptr,
        256 - sprite_data_address
    );
    if (sprite_data_address)
        std::memcpy(
            sprite_memory.data(),
            page_ptr + (256 - sprite_data_address),
            sprite_data_address
        );
    invalidate_sprite_rows();
}

void PPU::control(NES_Byte ctrl) {
    is_interrupting = ctrl & 0x80;
    is_long_sprites = ctrl & 0x20;
    background_page = static_cast<CharacterPage>(!!(ctrl & 0x10));
    sprite_page = static_cast<CharacterPage>(!!(ctrl & 0x8));
    background_tile_cache_valid = false;
    invalidate_sprite_rows();
    if (ctrl & 0x4)
        data_address_increment = 0x20;
    else
        data_address_increment = 1;
    // baseNameTable = (ctrl & 0x3) * 0x400 + 0x2000;
    // Set the nametable in the temp address, this will be reflected in the
    // data address during rendering
    // v-- Unset
    temp_address &= ~0xc00;
    // v-- Set according to ctrl bits
    temp_address |= (ctrl & 0x3) << 10;
}

void PPU::set_mask(NES_Byte mask) {
    is_hiding_edge_background = !(mask & 0x2);
    is_hiding_edge_sprites = !(mask & 0x4);
    is_showing_background = mask & 0x8;
    is_showing_sprites = mask & 0x10;
    background_tile_cache_valid = false;
    invalidate_sprite_rows();
}

NES_Byte PPU::get_status() {
    NES_Byte status = is_sprite_zero_hit << 6 | is_vblank << 7;
    // data_address = 0;
    is_vblank = false;
    is_first_write = true;
    return status;
}

void PPU::set_data_address(NES_Byte address) {
    // data_address = ((data_address << 8) & 0xff00) | address;
    if (is_first_write) {
        // Unset the upper byte
        temp_address &= ~0xff00;
        temp_address |= (address & 0x3f) << 8;
        is_first_write = false;
    } else {
        // Unset the lower byte;
        temp_address &= ~0xff;
        temp_address |= address;
        data_address = temp_address;
        is_first_write = true;
        background_tile_cache_valid = false;
    }
}

NES_Byte PPU::get_data(PictureBus& bus) {
    NES_Address read_address = data_address & 0x3fff;
    auto data = bus.read(read_address);
    data_address += data_address_increment;
    // Reads are delayed by one byte/read when address is in this range
    if (read_address < 0x3f00)
        // Return from the data buffer and store the current value in the buffer
        std::swap(data, data_buffer);
    return data;
}

void PPU::set_data(PictureBus& bus, NES_Byte data) {
    bus.write(data_address, data);
    background_tile_cache_valid = false;
    data_address += data_address_increment;
}

void PPU::set_scroll(NES_Byte scroll) {
    if (is_first_write) {
        temp_address &= ~0x1f;
        temp_address |= (scroll >> 3) & 0x1f;
        fine_x_scroll = scroll & 0x7;
        is_first_write = false;
        background_tile_cache_valid = false;
    } else {
        temp_address &= ~0x73e0;
        temp_address |= ((scroll & 0x7) << 12) | ((scroll & 0xf8) << 2);
        is_first_write = true;
        background_tile_cache_valid = false;
    }
}

}  // namespace NES
