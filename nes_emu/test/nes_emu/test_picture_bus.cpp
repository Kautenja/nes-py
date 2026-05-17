//  Program:      nes-py
//  File:         test_picture_bus.cpp
//  Description:  Catch2 PPU and picture-bus characterization tests
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include <vector>
#include <catch2/catch_test_macros.hpp>
#include "nes_emu/test/nes_emu/support/test_mappers.hpp"

namespace {

bool pattern_table_addresses_route_to_mapper_chr() {
    NESTest::PictureBusTestMapper mapper;
    NES::PictureBus bus;
    bus.set_mapper(&mapper);
    bus.write(0x0010, 0x42);
    bus.write(0x1f23, 0x99);
    return (
        bus.read(0x0010) == 0x42 &&
        bus.read(0x4010) == 0x42 &&
        bus.read(0x5f23) == 0x99
    );
}

bool vertical_nametable_mirroring_matches_address_aliases() {
    NESTest::PictureBusTestMapper mapper(NES::VERTICAL);
    NES::PictureBus bus;
    bus.set_mapper(&mapper);
    bus.write(0x2002, 0x11);
    bus.write(0x2403, 0x22);
    bus.write(0x3004, 0x33);
    bus.write(0x3eff, 0x44);
    return (
        bus.read(0x2802) == 0x11 &&
        bus.read(0x2c03) == 0x22 &&
        bus.read(0x2004) == 0x33 &&
        bus.read(0x2eff) == 0x44
    );
}

bool four_screen_mirroring_keeps_all_tables_separate() {
    NESTest::PictureBusTestMapper mapper(NES::FOUR_SCREEN);
    NES::PictureBus bus;
    bus.set_mapper(&mapper);
    bus.write(0x2001, 0x10);
    bus.write(0x2401, 0x20);
    bus.write(0x2801, 0x30);
    bus.write(0x2c01, 0x40);
    return (
        bus.read(0x2001) == 0x10 &&
        bus.read(0x2401) == 0x20 &&
        bus.read(0x2801) == 0x30 &&
        bus.read(0x2c01) == 0x40
    );
}

bool one_screen_mirroring_selects_lower_and_higher_pages() {
    NESTest::PictureBusTestMapper lower_mapper(NES::ONE_SCREEN_LOWER);
    NES::PictureBus lower_bus;
    lower_bus.set_mapper(&lower_mapper);
    lower_bus.write(0x2006, 0x55);

    NESTest::PictureBusTestMapper higher_mapper(NES::ONE_SCREEN_HIGHER);
    NES::PictureBus higher_bus;
    higher_bus.set_mapper(&higher_mapper);
    higher_bus.write(0x2407, 0x66);
    return (
        lower_bus.read(0x2406) == 0x55 &&
        lower_bus.read(0x2c06) == 0x55 &&
        higher_bus.read(0x2007) == 0x66 &&
        higher_bus.read(0x2c07) == 0x66
    );
}

bool palette_mirroring_normalizes_universal_background_addresses() {
    NES::PictureBus bus;
    bus.write(0x3f00, 0x01);
    bus.write(0x3f04, 0x04);
    bus.write(0x3f08, 0x08);
    bus.write(0x3f0c, 0x0c);
    bool mirrored = (
        bus.read(0x3f10) == 0x01 &&
        bus.read(0x3f14) == 0x04 &&
        bus.read(0x3f18) == 0x08 &&
        bus.read(0x3f1c) == 0x0c
    );
    bus.write(0x3f14, 0x24);
    bus.write(0x3f20, 0x30);
    return (
        mirrored &&
        bus.read(0x3f04) == 0x24 &&
        bus.read(0x3f00) == 0x30
    );
}

bool final_palette_address_mirrors_3f1f() {
    NES::PictureBus bus;
    bus.write(0x3f1f, 0x5f);
    return bus.read(0x3fff) == 0x5f;
}

bool ppudata_buffer_uses_original_address_boundary() {
    NESTest::PictureBusTestMapper mapper;
    NES::PictureBus bus;
    NES::PPU ppu;
    bus.set_mapper(&mapper);
    ppu.reset();

    bus.write(0x2eff, 0x77);
    ppu.set_data_address(0x3e);
    ppu.set_data_address(0xff);
    bool delayed_at_boundary = ppu.get_data(bus) == 0x00;

    bus.write(0x3f00, 0x56);
    ppu.set_data_address(0x3f);
    ppu.set_data_address(0x00);
    bool palette_immediate = ppu.get_data(bus) == 0x56;

    return delayed_at_boundary && palette_immediate;
}

bool ppu_reset_clears_latches_oam_and_screen() {
    NESTest::PictureBusTestMapper mapper;
    NES::PictureBus bus;
    NES::PPU ppu;
    bus.set_mapper(&mapper);
    ppu.reset();

    bus.write(0x2000, 0x55);
    ppu.set_data_address(0x20);
    ppu.set_data_address(0x00);
    ppu.get_data(bus);
    ppu.set_OAM_address(0x00);
    ppu.set_OAM_data(0x88);
    ppu.get_screen_buffer()[0] = 0x00ffffff;

    ppu.reset();
    bus.write(0x2000, 0x66);
    ppu.set_data_address(0x20);
    ppu.set_data_address(0x00);
    NES::NES_Byte first_buffered_read = ppu.get_data(bus);
    ppu.set_OAM_address(0x00);
    return (
        first_buffered_read == 0x00 &&
        ppu.get_OAM_data() == 0x00 &&
        ppu.get_screen_buffer()[0] == 0x00000000
    );
}

bool ppu_render_pipeline_emits_expected_mapper_hook_sequence() {
    NESTest::PPUHookMapper mapper;
    NES::PictureBus bus;
    NES::PPU ppu;
    std::vector<NES::NES_Address> expected = {
        0x2000, 0x0000, 0x0008, 0x23c0,
        0x2000, 0x0000, 0x0008, 0x23c0,
    };
    bus.set_mapper(&mapper);
    ppu.reset();
    for (int cycle = 0; cycle < 342; ++cycle)
        ppu.cycle(bus);
    ppu.cycle(bus);
    ppu.cycle(bus);
    return (
        mapper.address_observations == 8 &&
        mapper.read_observations == 8 &&
        mapper.last_address == 0x23c0 &&
        mapper.address_sequence == expected
    );
}

}  // namespace

TEST_CASE("picture bus and PPU addressing behavior is stable", "[ppu]") {
    REQUIRE(pattern_table_addresses_route_to_mapper_chr());
    REQUIRE(vertical_nametable_mirroring_matches_address_aliases());
    REQUIRE(four_screen_mirroring_keeps_all_tables_separate());
    REQUIRE(one_screen_mirroring_selects_lower_and_higher_pages());
    REQUIRE(palette_mirroring_normalizes_universal_background_addresses());
    REQUIRE(final_palette_address_mirrors_3f1f());
    REQUIRE(ppudata_buffer_uses_original_address_boundary());
    REQUIRE(ppu_reset_clears_latches_oam_and_screen());
    REQUIRE(ppu_render_pipeline_emits_expected_mapper_hook_sequence());
}
