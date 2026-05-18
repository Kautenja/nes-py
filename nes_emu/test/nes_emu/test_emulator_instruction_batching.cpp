//  Program:      nes-py
//  File:         test_emulator_instruction_batching.cpp
//  Description:  Catch2 frame-level CPU instruction batching coverage
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <string>
#include <vector>
#include "nes_emu/emulator.hpp"
#include "nes_emu/test/nes_emu/support/emulator_inspector.hpp"

namespace {

std::string game_path(const std::string& filename) {
    return "nes_py/tests/games/" + filename;
}

bool file_exists(const std::string& path) {
    std::ifstream stream(path.c_str(), std::ios::binary);
    return stream.good();
}

bool cpu_snapshots_equal(
    const NES::CPU::Snapshot& a,
    const NES::CPU::Snapshot& b
) {
    return (
        a.register_PC == b.register_PC &&
        a.register_SP == b.register_SP &&
        a.register_A == b.register_A &&
        a.register_X == b.register_X &&
        a.register_Y == b.register_Y &&
        a.flags == b.flags &&
        a.skip_cycles == b.skip_cycles &&
        a.cycles == b.cycles
    );
}

bool ppu_snapshots_equal(
    const NES::PPU::Snapshot& a,
    const NES::PPU::Snapshot& b
) {
    return (
        a.sprite_memory == b.sprite_memory &&
        a.scanline_sprites == b.scanline_sprites &&
        a.scanline_sprite_count == b.scanline_sprite_count &&
        a.scanline_sprite_rows_cached == b.scanline_sprite_rows_cached &&
        a.scanline_sprite_rows_generation == b.scanline_sprite_rows_generation &&
        a.pipeline_state == b.pipeline_state &&
        a.cycles == b.cycles &&
        a.scanline == b.scanline &&
        a.is_even_frame == b.is_even_frame &&
        a.is_vblank == b.is_vblank &&
        a.is_sprite_zero_hit == b.is_sprite_zero_hit &&
        a.data_address == b.data_address &&
        a.temp_address == b.temp_address &&
        a.fine_x_scroll == b.fine_x_scroll &&
        a.is_first_write == b.is_first_write &&
        a.data_buffer == b.data_buffer &&
        a.sprite_data_address == b.sprite_data_address &&
        a.is_showing_sprites == b.is_showing_sprites &&
        a.is_showing_background == b.is_showing_background &&
        a.is_hiding_edge_sprites == b.is_hiding_edge_sprites &&
        a.is_hiding_edge_background == b.is_hiding_edge_background &&
        a.is_long_sprites == b.is_long_sprites &&
        a.is_interrupting == b.is_interrupting &&
        a.background_page == b.background_page &&
        a.sprite_page == b.sprite_page &&
        a.data_address_increment == b.data_address_increment &&
        a.background_tile_cache_valid == b.background_tile_cache_valid &&
        a.background_tile_cache_address == b.background_tile_cache_address &&
        a.background_tile_cache_page == b.background_tile_cache_page &&
        a.background_tile_cache_generation == b.background_tile_cache_generation &&
        a.background_tile_cache_low == b.background_tile_cache_low &&
        a.background_tile_cache_high == b.background_tile_cache_high &&
        a.background_tile_cache_attribute == b.background_tile_cache_attribute &&
        a.background_tile_cache_low_bits == b.background_tile_cache_low_bits &&
        a.background_tile_cache_high_bits == b.background_tile_cache_high_bits &&
        a.background_tile_cache_palette_high ==
            b.background_tile_cache_palette_high &&
        a.background_tile_cache_opaque_mask ==
            b.background_tile_cache_opaque_mask &&
        a.screen == b.screen
    );
}

void require_emulator_states_equal(
    NES::Emulator& cycle_emulator,
    NES::Emulator& batched_emulator,
    const std::string& phase
) {
    INFO(phase);
    REQUIRE(cpu_snapshots_equal(
        NES::EmulatorInspector::cpu_snapshot(cycle_emulator),
        NES::EmulatorInspector::cpu_snapshot(batched_emulator)
    ));
    REQUIRE(
        NES::EmulatorInspector::bus_state(cycle_emulator).ram ==
        NES::EmulatorInspector::bus_state(batched_emulator).ram
    );
    REQUIRE(ppu_snapshots_equal(
        NES::EmulatorInspector::ppu_snapshot(cycle_emulator),
        NES::EmulatorInspector::ppu_snapshot(batched_emulator)
    ));
}

void compare_cycle_and_batched_frames(
    const std::string& path,
    int expected_mapper
) {
    REQUIRE(file_exists(path));
    NES::Emulator cycle_emulator(path);
    NES::Emulator batched_emulator(path);
    REQUIRE(cycle_emulator.get_mapper_number() == expected_mapper);
    REQUIRE(batched_emulator.get_mapper_number() == expected_mapper);
    REQUIRE(NES::EmulatorInspector::uses_instruction_batching(batched_emulator));

    cycle_emulator.reset();
    batched_emulator.reset();
    require_emulator_states_equal(
        cycle_emulator,
        batched_emulator,
        path + " after reset"
    );

    for (int frame = 0; frame < 3; ++frame) {
        NES::EmulatorInspector::step_cycle_by_cycle(cycle_emulator);
        NES::EmulatorInspector::step_instruction_batched(batched_emulator);
        require_emulator_states_equal(
            cycle_emulator,
            batched_emulator,
            path + " frame " + std::to_string(frame)
        );
    }

    cycle_emulator.backup();
    batched_emulator.backup();
    NES::EmulatorInspector::step_cycle_by_cycle(cycle_emulator);
    NES::EmulatorInspector::step_instruction_batched(batched_emulator);
    require_emulator_states_equal(
        cycle_emulator,
        batched_emulator,
        path + " after backup continuation"
    );

    NES::EmulatorInspector::step_cycle_by_cycle(cycle_emulator);
    NES::EmulatorInspector::step_instruction_batched(batched_emulator);
    require_emulator_states_equal(
        cycle_emulator,
        batched_emulator,
        path + " after second continuation"
    );

    cycle_emulator.restore();
    batched_emulator.restore();
    require_emulator_states_equal(
        cycle_emulator,
        batched_emulator,
        path + " after restore"
    );

    NES::EmulatorInspector::step_cycle_by_cycle(cycle_emulator);
    NES::EmulatorInspector::step_instruction_batched(batched_emulator);
    require_emulator_states_equal(
        cycle_emulator,
        batched_emulator,
        path + " after restored continuation"
    );
}

void compare_default_and_cycle_path_for_hooked_mapper(
    const std::string& path,
    int expected_mapper
) {
    REQUIRE(file_exists(path));
    NES::Emulator cycle_emulator(path);
    NES::Emulator default_emulator(path);
    REQUIRE(cycle_emulator.get_mapper_number() == expected_mapper);
    REQUIRE(default_emulator.get_mapper_number() == expected_mapper);
    REQUIRE_FALSE(NES::EmulatorInspector::uses_instruction_batching(
        default_emulator
    ));

    cycle_emulator.reset();
    default_emulator.reset();
    NES::EmulatorInspector::step_cycle_by_cycle(cycle_emulator);
    default_emulator.step();
    require_emulator_states_equal(
        cycle_emulator,
        default_emulator,
        path + " hooked mapper default step"
    );
}

}  // namespace

TEST_CASE(
    "instruction-batched stepping matches cycle-by-cycle frames",
    "[emulator][batching]"
) {
    compare_cycle_and_batched_frames(
        game_path("super-mario-bros-1.nes"),
        0
    );
    compare_cycle_and_batched_frames(
        game_path("the-legend-of-zelda.nes"),
        1
    );
    compare_cycle_and_batched_frames(
        game_path("mega-man.nes"),
        2
    );
    compare_cycle_and_batched_frames(
        game_path("adventure-island.nes"),
        3
    );
}

TEST_CASE(
    "CPU-cycle-hooked mappers keep the cycle-by-cycle frame path",
    "[emulator][batching][mapper]"
) {
    compare_default_and_cycle_path_for_hooked_mapper(
        game_path("castlevania-iii-draculas-curse.nes"),
        5
    );
    compare_default_and_cycle_path_for_hooked_mapper(
        game_path("batman-return-of-the-joker.nes"),
        69
    );
}
