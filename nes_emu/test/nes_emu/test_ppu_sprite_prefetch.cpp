//  Program:      nes-py
//  File:         test_ppu_sprite_prefetch.cpp
//  Description:  Catch2 coverage for PPU sprite row prefetching
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include <catch2/catch_test_macros.hpp>
#include "nes_emu/palette.hpp"
#include "nes_emu/picture_bus.hpp"
#include "nes_emu/ppu.hpp"
#include "nes_emu/test/nes_emu/support/test_mappers.hpp"

namespace {

const int PRE_RENDER_CYCLES = 342;
const int RENDER_SCANLINE_CYCLES = NES::SCANLINE_END_CYCLE;
const int FIRST_SPRITE_SCANLINE_CYCLES =
    PRE_RENDER_CYCLES + RENDER_SCANLINE_CYCLES;
const int PPU_CYCLES_PER_FRAME = 29781 * 3;

class CountingSpriteMapper : public NESTest::ProgramTestMapper {
 public:
    int chr_reads;

    CountingSpriteMapper() : NESTest::ProgramTestMapper(), chr_reads(0) { }

    inline NES::NES_Byte readCHR(NES::NES_Address address) {
        ++chr_reads;
        return NESTest::ProgramTestMapper::readCHR(address);
    }
};

void run_cycles(NES::PPU& ppu, NES::PictureBus& bus, int cycles) {
    for (int cycle = 0; cycle < cycles; ++cycle)
        ppu.cycle(bus);
}

void hide_all_sprites(NES::PPU& ppu) {
    ppu.set_OAM_address(0);
    for (int index = 0; index < 64; ++index) {
        ppu.set_OAM_data(0xff);
        ppu.set_OAM_data(0x00);
        ppu.set_OAM_data(0x00);
        ppu.set_OAM_data(0x00);
    }
    ppu.set_OAM_address(0);
}

void write_sprite(
    NES::PPU& ppu,
    NES::NES_Byte index,
    NES::NES_Byte y,
    NES::NES_Byte tile,
    NES::NES_Byte attribute,
    NES::NES_Byte x
) {
    ppu.set_OAM_address(static_cast<NES::NES_Byte>(index * 4));
    ppu.set_OAM_data(y);
    ppu.set_OAM_data(tile);
    ppu.set_OAM_data(attribute);
    ppu.set_OAM_data(x);
    ppu.set_OAM_address(0);
}

void seed_common_palette(NES::PictureBus& bus) {
    bus.write(0x3f00, 0x00);
    bus.write(0x3f01, 0x01);
    bus.write(0x3f11, 0x21);
    bus.write(0x3f15, 0x25);
}

void seed_opaque_background(NES::PictureBus& bus) {
    for (NES::NES_Address row = 0; row < 8; ++row) {
        bus.write(row, 0xff);
        bus.write(static_cast<NES::NES_Address>(row + 8), 0x00);
    }
}

NES::NES_Pixel screen_pixel(NES::PPU& ppu, int y, int x) {
    return ppu.get_screen_buffer()[y * NES::SCANLINE_VISIBLE_DOTS + x];
}

}  // namespace

TEST_CASE(
    "sprite row prefetch reads each selected sprite row once",
    "[ppu][sprite]"
) {
    CountingSpriteMapper mapper;
    NES::PictureBus bus;
    NES::PPU ppu;

    bus.set_mapper(&mapper);
    REQUIRE(bus.can_prefetch_sprite_rows());
    ppu.reset();
    ppu.control(0x00);
    ppu.set_mask(0x14);
    hide_all_sprites(ppu);
    seed_common_palette(bus);
    bus.write(0x0010, 0xff);
    bus.write(0x0018, 0x00);
    for (NES::NES_Byte index = 0; index < 8; ++index)
        write_sprite(
            ppu,
            index,
            0x00,
            0x01,
            0x00,
            static_cast<NES::NES_Byte>(index * 16)
        );

    mapper.chr_reads = 0;
    run_cycles(ppu, bus, FIRST_SPRITE_SCANLINE_CYCLES);
    REQUIRE(mapper.chr_reads == 16);

    run_cycles(ppu, bus, 8);
    REQUIRE(mapper.chr_reads == 16);
    REQUIRE(screen_pixel(ppu, 1, 0) == NES::PALETTE[0x21]);
    REQUIRE(screen_pixel(ppu, 1, 7) == NES::PALETTE[0x21]);
}

TEST_CASE(
    "sprite row prefetch preserves flips, priority, transparency, and hit",
    "[ppu][sprite]"
) {
    NESTest::ProgramTestMapper mapper;
    NES::PictureBus bus;
    NES::PPU ppu;

    bus.set_mapper(&mapper);
    ppu.reset();
    ppu.control(0x00);
    ppu.set_mask(0x1e);
    hide_all_sprites(ppu);
    seed_common_palette(bus);
    seed_opaque_background(bus);

    mapper.writeCHR(0x0010, 0x80);
    mapper.writeCHR(0x0018, 0x00);
    mapper.writeCHR(0x0027, 0x80);
    mapper.writeCHR(0x002f, 0x00);
    mapper.writeCHR(0x0030, 0x80);
    mapper.writeCHR(0x0038, 0x00);
    mapper.writeCHR(0x0050, 0x80);
    mapper.writeCHR(0x0058, 0x00);
    mapper.writeCHR(0x0060, 0x80);
    mapper.writeCHR(0x0068, 0x00);

    write_sprite(ppu, 0, 0x00, 0x01, 0x40, 10);
    write_sprite(ppu, 1, 0x00, 0x02, 0x80, 30);
    write_sprite(ppu, 2, 0x00, 0x03, 0x20, 50);
    write_sprite(ppu, 3, 0x00, 0x04, 0x00, 70);
    write_sprite(ppu, 4, 0x00, 0x05, 0x00, 90);
    write_sprite(ppu, 5, 0x00, 0x06, 0x01, 90);
    write_sprite(ppu, 6, 0x00, 0x04, 0x00, 130);
    write_sprite(ppu, 7, 0x00, 0x04, 0x00, 150);

    run_cycles(ppu, bus, PPU_CYCLES_PER_FRAME);

    REQUIRE(screen_pixel(ppu, 1, 10) == NES::PALETTE[0x01]);
    REQUIRE(screen_pixel(ppu, 1, 17) == NES::PALETTE[0x21]);
    REQUIRE(screen_pixel(ppu, 1, 30) == NES::PALETTE[0x21]);
    REQUIRE(screen_pixel(ppu, 1, 50) == NES::PALETTE[0x01]);
    REQUIRE(screen_pixel(ppu, 1, 70) == NES::PALETTE[0x01]);
    REQUIRE(screen_pixel(ppu, 1, 90) == NES::PALETTE[0x21]);
    REQUIRE((ppu.get_status() & 0x40) == 0x40);
}

TEST_CASE("sprite row prefetch preserves 8x16 sprite rows", "[ppu][sprite]") {
    NESTest::ProgramTestMapper mapper;
    NES::PictureBus bus;
    NES::PPU ppu;

    bus.set_mapper(&mapper);
    ppu.reset();
    ppu.control(0x20);
    ppu.set_mask(0x14);
    hide_all_sprites(ppu);
    seed_common_palette(bus);

    mapper.writeCHR(0x1020, 0x80);
    mapper.writeCHR(0x1028, 0x00);
    mapper.writeCHR(0x1030, 0x80);
    mapper.writeCHR(0x1038, 0x00);
    for (NES::NES_Byte index = 0; index < 8; ++index)
        write_sprite(
            ppu,
            index,
            0x00,
            0x03,
            0x00,
            static_cast<NES::NES_Byte>(12 + index * 24)
        );

    run_cycles(ppu, bus, PPU_CYCLES_PER_FRAME);

    REQUIRE(screen_pixel(ppu, 1, 12) == NES::PALETTE[0x21]);
    REQUIRE(screen_pixel(ppu, 9, 12) == NES::PALETTE[0x21]);
}

TEST_CASE(
    "sprite row prefetch is disabled for mapper PPU observers",
    "[ppu][sprite][mapper]"
) {
    NESTest::PPUHookMapper mapper;
    NES::PictureBus bus;
    NES::PPU ppu;

    bus.set_mapper(&mapper);
    REQUIRE_FALSE(bus.can_prefetch_sprite_rows());
    ppu.reset();
    ppu.control(0x00);
    ppu.set_mask(0x14);
    hide_all_sprites(ppu);
    write_sprite(ppu, 0, 0x00, 0x01, 0x00, 0x00);

    run_cycles(ppu, bus, FIRST_SPRITE_SCANLINE_CYCLES);
    REQUIRE(mapper.address_observations == 0);

    run_cycles(ppu, bus, 2);
    REQUIRE(mapper.address_observations == 4);
    REQUIRE(mapper.read_observations == 4);
    REQUIRE(mapper.address_sequence[0] == 0x0010);
    REQUIRE(mapper.address_sequence[1] == 0x0018);
    REQUIRE(mapper.address_sequence[2] == 0x0010);
    REQUIRE(mapper.address_sequence[3] == 0x0018);
}
