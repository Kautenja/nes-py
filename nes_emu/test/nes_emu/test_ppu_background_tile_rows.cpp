//  Program:      nes-py
//  File:         test_ppu_background_tile_rows.cpp
//  Description:  Catch2 coverage for decoded background tile-row batching
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <vector>
#include "nes_emu/palette.hpp"
#include "nes_emu/picture_bus.hpp"
#include "nes_emu/ppu.hpp"
#include "nes_emu/test/nes_emu/support/test_mappers.hpp"

namespace {

const int PRE_RENDER_CYCLES = 342;
const int RENDER_SCANLINE_CYCLES = NES::SCANLINE_END_CYCLE;

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

void seed_palette(NES::PictureBus& bus) {
    for (NES::NES_Address index = 0; index < 0x10; ++index)
        bus.write(
            static_cast<NES::NES_Address>(0x3f00 + index),
            static_cast<NES::NES_Byte>(index)
        );
}

void set_tile_row(
    NES::PictureBus& bus,
    NES::NES_Byte tile,
    NES::NES_Byte fine_y,
    NES::NES_Byte low,
    NES::NES_Byte high
) {
    const NES::NES_Address address = static_cast<NES::NES_Address>(
        tile * 16 + fine_y
    );
    bus.write(address, low);
    bus.write(static_cast<NES::NES_Address>(address + 8), high);
}

NES::NES_Pixel screen_pixel(NES::PPU& ppu, int y, int x) {
    return ppu.get_screen_buffer()[y * NES::SCANLINE_VISIBLE_DOTS + x];
}

void prepare_scrolled_background(
    NES::PPU& ppu,
    NES::NES_Byte mask,
    NES::NES_Byte horizontal_scroll,
    NES::NES_Byte vertical_scroll
) {
    hide_all_sprites(ppu);
    ppu.set_scroll(horizontal_scroll);
    ppu.set_scroll(vertical_scroll);
    ppu.set_mask(mask);
}

class CacheablePPUAddressMapper : public NESTest::PictureBusTestMapper {
 public:
    int address_observations;
    std::vector<NES::NES_Address> address_sequence;

    CacheablePPUAddressMapper() :
        NESTest::PictureBusTestMapper(NES::VERTICAL),
        address_observations(0),
        address_sequence() { }

    inline void onPPUAddress(NES::NES_Address address) {
        ++address_observations;
        address_sequence.push_back(address);
    }

    inline bool observesPPUAddresses() const { return true; }

    inline bool allowsBackgroundTileCacheWithPPUAddressObservations() const {
        return true;
    }
};

class SpriteFetchPPUAddressMapper : public NESTest::PPUHookMapper {
 public:
    inline bool requiresPPUSpriteFetchAddressObservations() const {
        return true;
    }
};

}  // namespace

TEST_CASE(
    "background tile row batching preserves fine X scroll",
    "[ppu][background]"
) {
    NESTest::PictureBusTestMapper mapper(NES::VERTICAL);
    NES::PictureBus bus;
    NES::PPU ppu;

    bus.set_mapper(&mapper);
    ppu.reset();
    seed_palette(bus);
    prepare_scrolled_background(ppu, 0x0a, 0x03, 0x00);
    bus.write(0x2000, 0x00);
    bus.write(0x2001, 0x01);
    set_tile_row(bus, 0x00, 0, 0x10, 0x00);
    set_tile_row(bus, 0x01, 0, 0x80, 0x00);

    run_cycles(ppu, bus, PRE_RENDER_CYCLES + 6);

    REQUIRE(screen_pixel(ppu, 0, 0) == NES::PALETTE[0x01]);
    REQUIRE(screen_pixel(ppu, 0, 1) == NES::PALETTE[0x00]);
    REQUIRE(screen_pixel(ppu, 0, 5) == NES::PALETTE[0x01]);
}

TEST_CASE(
    "background tile row batching wraps coarse X across nametables",
    "[ppu][background]"
) {
    NESTest::PictureBusTestMapper mapper(NES::VERTICAL);
    NES::PictureBus bus;
    NES::PPU ppu;

    bus.set_mapper(&mapper);
    ppu.reset();
    seed_palette(bus);
    prepare_scrolled_background(ppu, 0x1e, 0xf8, 0x00);
    bus.write(0x201f, 0x02);
    bus.write(0x2400, 0x03);
    set_tile_row(bus, 0x02, 0, 0xff, 0x00);
    set_tile_row(bus, 0x03, 0, 0x00, 0xff);

    run_cycles(ppu, bus, PRE_RENDER_CYCLES + 9);

    REQUIRE(screen_pixel(ppu, 0, 0) == NES::PALETTE[0x01]);
    REQUIRE(screen_pixel(ppu, 0, 8) == NES::PALETTE[0x02]);
}

TEST_CASE(
    "background tile row batching preserves vertical fine-Y increments",
    "[ppu][background]"
) {
    NESTest::PictureBusTestMapper mapper(NES::VERTICAL);
    NES::PictureBus bus;
    NES::PPU ppu;

    bus.set_mapper(&mapper);
    ppu.reset();
    seed_palette(bus);
    prepare_scrolled_background(ppu, 0x1e, 0x00, 0x00);
    bus.write(0x2000, 0x04);
    set_tile_row(bus, 0x04, 0, 0x80, 0x00);
    set_tile_row(bus, 0x04, 1, 0x00, 0x80);

    run_cycles(
        ppu,
        bus,
        PRE_RENDER_CYCLES + RENDER_SCANLINE_CYCLES + 1
    );

    REQUIRE(screen_pixel(ppu, 0, 0) == NES::PALETTE[0x01]);
    REQUIRE(screen_pixel(ppu, 1, 0) == NES::PALETTE[0x02]);
}

TEST_CASE(
    "background tile row batching preserves attribute quadrants",
    "[ppu][background]"
) {
    NESTest::PictureBusTestMapper mapper(NES::VERTICAL);
    NES::PictureBus bus;
    NES::PPU ppu;

    bus.set_mapper(&mapper);
    ppu.reset();
    seed_palette(bus);
    prepare_scrolled_background(ppu, 0x1e, 0x00, 0x00);
    bus.write(0x2000, 0x04);
    bus.write(0x2002, 0x04);
    bus.write(0x2040, 0x04);
    bus.write(0x2042, 0x04);
    bus.write(0x23c0, 0xe4);
    set_tile_row(bus, 0x04, 0, 0x80, 0x00);

    run_cycles(
        ppu,
        bus,
        PRE_RENDER_CYCLES + RENDER_SCANLINE_CYCLES * 16 + 17
    );

    REQUIRE(screen_pixel(ppu, 0, 0) == NES::PALETTE[0x01]);
    REQUIRE(screen_pixel(ppu, 0, 16) == NES::PALETTE[0x05]);
    REQUIRE(screen_pixel(ppu, 16, 0) == NES::PALETTE[0x09]);
    REQUIRE(screen_pixel(ppu, 16, 16) == NES::PALETTE[0x0d]);
}

TEST_CASE(
    "background tile row batching preserves left-edge background masking",
    "[ppu][background]"
) {
    NESTest::PictureBusTestMapper mapper(NES::VERTICAL);
    NES::PictureBus bus;
    NES::PPU ppu;

    bus.set_mapper(&mapper);
    ppu.reset();
    seed_palette(bus);
    prepare_scrolled_background(ppu, 0x18, 0x00, 0x00);
    bus.write(0x2000, 0x00);
    bus.write(0x2001, 0x04);
    set_tile_row(bus, 0x04, 0, 0x80, 0x00);

    run_cycles(ppu, bus, PRE_RENDER_CYCLES + 9);

    REQUIRE(screen_pixel(ppu, 0, 0) == NES::PALETTE[0x00]);
    REQUIRE(screen_pixel(ppu, 0, 7) == NES::PALETTE[0x00]);
    REQUIRE(screen_pixel(ppu, 0, 8) == NES::PALETTE[0x01]);
}

TEST_CASE(
    "background tile row batching is disabled for mapper PPU observers",
    "[ppu][background][mapper]"
) {
    NESTest::PPUHookMapper mapper;
    NES::PictureBus bus;
    NES::PPU ppu;
    const std::vector<NES::NES_Address> expected = {
        0x2000, 0x0000, 0x0008, 0x23c0,
        0x2000, 0x0000, 0x0008, 0x23c0,
    };

    bus.set_mapper(&mapper);
    REQUIRE(bus.has_mapper_ppu_observers());
    ppu.reset();

    run_cycles(ppu, bus, PRE_RENDER_CYCLES + 2);

    REQUIRE(mapper.address_observations == 8);
    REQUIRE(mapper.read_observations == 8);
    REQUIRE(mapper.address_sequence == expected);
}

TEST_CASE(
    "background tile row batching can preserve mapper PPU address observations",
    "[ppu][background][mapper]"
) {
    CacheablePPUAddressMapper mapper;
    NES::PictureBus bus;
    NES::PPU ppu;
    const std::vector<NES::NES_Address> expected = {
        0x2000, 0x0000, 0x0008, 0x23c0,
    };

    bus.set_mapper(&mapper);
    REQUIRE(bus.has_mapper_ppu_observers());
    REQUIRE(bus.can_cache_background_tile_rows());
    ppu.reset();

    run_cycles(ppu, bus, PRE_RENDER_CYCLES + 2);

    REQUIRE(mapper.address_observations == 4);
    REQUIRE(mapper.address_sequence == expected);
}

TEST_CASE(
    "PPU emits dummy sprite fetches for mappers that observe sprite A12",
    "[ppu][sprite][mapper]"
) {
    SpriteFetchPPUAddressMapper mapper;
    NES::PictureBus bus;
    NES::PPU ppu;

    bus.set_mapper(&mapper);
    ppu.reset();
    hide_all_sprites(ppu);
    ppu.control(0x08);

    run_cycles(
        ppu,
        bus,
        PRE_RENDER_CYCLES + NES::SCANLINE_VISIBLE_DOTS + 5
    );

    const auto low = std::find(
        mapper.address_sequence.begin(),
        mapper.address_sequence.end(),
        0x1ff0
    );
    REQUIRE(low != mapper.address_sequence.end());
    REQUIRE(std::next(low) != mapper.address_sequence.end());
    REQUIRE(*std::next(low) == 0x1ff8);
}
