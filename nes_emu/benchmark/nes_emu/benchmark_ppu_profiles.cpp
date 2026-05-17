//  Program:      nes-py
//  File:         benchmark_ppu_profiles.cpp
//  Description:  Catch2 native PPU rendering and ROM frame benchmarks
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include "nes_emu/emulator.hpp"
#include "nes_emu/mapper_factory.hpp"
#include "nes_emu/picture_bus.hpp"
#include "nes_emu/ppu.hpp"
#include "nes_emu/test/nes_emu/support/synthetic_rom.hpp"
#include "nes_emu/test/nes_emu/support/test_mappers.hpp"

namespace {

const int CPU_CYCLES_PER_FRAME = 29781;
const int PPU_CYCLES_PER_FRAME = CPU_CYCLES_PER_FRAME * 3;
const int CHR_STRESS_READS = 8192;

volatile std::uint64_t benchmark_ppu_sink = 0;

bool file_exists(const std::string& path) {
    std::ifstream stream(path.c_str(), std::ios::binary);
    return stream.good();
}

void consume_screen(NES::NES_Pixel* screen) {
    benchmark_ppu_sink ^= screen[0];
    benchmark_ppu_sink ^= screen[
        NES::VISIBLE_SCANLINES * NES::SCANLINE_VISIBLE_DOTS - 1
    ];
}

void seed_picture_memory(
    NESTest::ProgramTestMapper& mapper,
    NES::PictureBus& bus
) {
    for (NES::NES_Address address = 0; address < 0x2000; ++address) {
        NES::NES_Byte value = static_cast<NES::NES_Byte>(
            ((address * 13) ^ (address >> 3) ^ 0xa5) & 0xff
        );
        mapper.writeCHR(address, value);
    }

    for (NES::NES_Address address = 0x2000; address < 0x23c0; ++address) {
        NES::NES_Byte tile = static_cast<NES::NES_Byte>(
            (address + (address >> 5)) & 0xff
        );
        bus.write(address, tile);
    }

    for (NES::NES_Address address = 0x23c0; address < 0x2400; ++address) {
        NES::NES_Byte attribute = static_cast<NES::NES_Byte>(
            ((address << 1) ^ 0x55) & 0xff
        );
        bus.write(address, attribute);
    }

    for (NES::NES_Address address = 0x3f00; address < 0x3f20; ++address) {
        bus.write(
            address,
            static_cast<NES::NES_Byte>((address - 0x3f00) & 0x3f)
        );
    }
}

void seed_sprite_memory(NES::PPU& ppu) {
    ppu.set_OAM_address(0);
    for (int index = 0; index < 64; ++index) {
        NES::NES_Byte y = static_cast<NES::NES_Byte>((index / 8) * 28);
        NES::NES_Byte tile = static_cast<NES::NES_Byte>(index * 3);
        NES::NES_Byte attribute = static_cast<NES::NES_Byte>(index & 0x03);
        NES::NES_Byte x = static_cast<NES::NES_Byte>((index % 8) * 31);
        ppu.set_OAM_data(y);
        ppu.set_OAM_data(tile);
        ppu.set_OAM_data(attribute);
        ppu.set_OAM_data(x);
    }
}

class SyntheticPPUHarness {
 private:
    NESTest::ProgramTestMapper mapper;
    NES::PictureBus bus;
    NES::PPU ppu;
    NES::NES_Byte mask;
    bool sprite_heavy;

 public:
    SyntheticPPUHarness(NES::NES_Byte mask, bool sprite_heavy) :
        mapper(),
        bus(),
        ppu(),
        mask(mask),
        sprite_heavy(sprite_heavy) {
        bus.set_mapper(&mapper);
        seed_picture_memory(mapper, bus);
    }

    void run_frame() {
        ppu.reset();
        ppu.control(0x00);
        ppu.set_scroll(0x00);
        ppu.set_scroll(0x00);
        ppu.set_mask(mask);
        if (sprite_heavy)
            seed_sprite_memory(ppu);
        for (int cycle = 0; cycle < PPU_CYCLES_PER_FRAME; ++cycle)
            ppu.cycle(bus);
        consume_screen(ppu.get_screen_buffer());
    }
};

class MapperCHRReadHarness {
 private:
    NESTest::TemporaryROM rom;
    NES::Cartridge cartridge;
    std::unique_ptr<NES::Mapper> mapper;
    NES::PictureBus bus;

 public:
    MapperCHRReadHarness(
        const std::string& name,
        std::uint16_t mapper_id,
        std::size_t prg_banks,
        std::size_t chr_banks
    ) :
        rom(name, mapper_id, prg_banks, chr_banks),
        cartridge(rom.load()),
        mapper(NES::MapperFactory(&cartridge)),
        bus() {
        REQUIRE(mapper != nullptr);
        bus.set_mapper(mapper.get());
        for (NES::NES_Address address = 0; address < 0x2000; ++address) {
            bus.write(
                address,
                static_cast<NES::NES_Byte>((address * 7 + mapper_id) & 0xff)
            );
        }
    }

    void run_reads() {
        NES::NES_Byte value = 0;
        for (int index = 0; index < CHR_STRESS_READS; ++index) {
            NES::NES_Address address = static_cast<NES::NES_Address>(
                (index * 37) & 0x1fff
            );
            value ^= bus.read(address);
        }
        benchmark_ppu_sink ^= value;
    }
};

class ROMFrameHarness {
 private:
    std::string path;
    int expected_mapper;
    std::unique_ptr<NES::Emulator> emulator;

 public:
    ROMFrameHarness(const std::string& path, int expected_mapper) :
        path(path),
        expected_mapper(expected_mapper),
        emulator() {
        REQUIRE(file_exists(path));
        emulator.reset(new NES::Emulator(path));
        REQUIRE(emulator->get_mapper_number() == expected_mapper);
        emulator->reset();
        emulator->backup();
    }

    void run_frame() {
        emulator->restore();
        emulator->step();
        consume_screen(emulator->get_screen_buffer());
        benchmark_ppu_sink ^= static_cast<std::uint64_t>(expected_mapper);
    }
};

SyntheticPPUHarness& render_off_harness() {
    static SyntheticPPUHarness harness(0x00, false);
    return harness;
}

SyntheticPPUHarness& background_only_harness() {
    static SyntheticPPUHarness harness(0x0a, false);
    return harness;
}

SyntheticPPUHarness& sprite_heavy_harness() {
    static SyntheticPPUHarness harness(0x1e, true);
    return harness;
}

MapperCHRReadHarness& mapper_0_chr_harness() {
    static MapperCHRReadHarness harness("benchmark_mapper_000_nrom", 0, 2, 1);
    return harness;
}

MapperCHRReadHarness& mapper_1_chr_harness() {
    static MapperCHRReadHarness harness("benchmark_mapper_001_sxrom", 1, 4, 2);
    return harness;
}

MapperCHRReadHarness& mapper_2_chr_harness() {
    static MapperCHRReadHarness harness("benchmark_mapper_002_uxrom", 2, 4, 0);
    return harness;
}

MapperCHRReadHarness& mapper_3_chr_harness() {
    static MapperCHRReadHarness harness("benchmark_mapper_003_cnrom", 3, 2, 4);
    return harness;
}

ROMFrameHarness& mapper_0_rom_harness() {
    static ROMFrameHarness harness(
        "nes_py/tests/games/super-mario-bros-1.nes",
        0
    );
    return harness;
}

ROMFrameHarness& mapper_1_rom_harness() {
    static ROMFrameHarness harness(
        "nes_py/tests/games/the-legend-of-zelda.nes",
        1
    );
    return harness;
}

ROMFrameHarness& mapper_2_rom_harness() {
    static ROMFrameHarness harness(
        "nes_py/tests/games/mega-man.nes",
        2
    );
    return harness;
}

ROMFrameHarness& mapper_3_rom_harness() {
    static ROMFrameHarness harness(
        "nes_py/tests/games/adventure-island.nes",
        3
    );
    return harness;
}

}  // namespace

TEST_CASE("native PPU rendering mode benchmark profiles", "[benchmark][ppu]") {
    BENCHMARK(
        "ppu mapper=0 synthetic=render-off render-mode=mask-off "
        "operation=one-ppu-frame"
    ) {
        render_off_harness().run_frame();
    };

    BENCHMARK(
        "ppu mapper=0 synthetic=background-only render-mode=background "
        "operation=one-ppu-frame"
    ) {
        background_only_harness().run_frame();
    };

    BENCHMARK(
        "ppu mapper=0 synthetic=sprite-heavy render-mode=background+sprites "
        "operation=one-ppu-frame"
    ) {
        sprite_heavy_harness().run_frame();
    };
}

TEST_CASE("native mapper CHR read benchmark profiles", "[benchmark][ppu]") {
    BENCHMARK(
        "ppu mapper=0 rom=synthetic-nrom render-mode=chr-read-stress "
        "operation=8192-picture-bus-reads"
    ) {
        mapper_0_chr_harness().run_reads();
    };

    BENCHMARK(
        "ppu mapper=1 rom=synthetic-sxrom render-mode=chr-read-stress "
        "operation=8192-picture-bus-reads"
    ) {
        mapper_1_chr_harness().run_reads();
    };

    BENCHMARK(
        "ppu mapper=2 rom=synthetic-uxrom render-mode=chr-read-stress "
        "operation=8192-picture-bus-reads"
    ) {
        mapper_2_chr_harness().run_reads();
    };

    BENCHMARK(
        "ppu mapper=3 rom=synthetic-cnrom render-mode=chr-read-stress "
        "operation=8192-picture-bus-reads"
    ) {
        mapper_3_chr_harness().run_reads();
    };
}

TEST_CASE("native representative ROM frame benchmark profiles", "[benchmark][ppu]") {
    BENCHMARK(
        "ppu mapper=0 rom=super-mario-bros-1 render-mode=full-frame "
        "operation=restore-and-step-one-frame"
    ) {
        mapper_0_rom_harness().run_frame();
    };

    BENCHMARK(
        "ppu mapper=1 rom=the-legend-of-zelda render-mode=full-frame "
        "operation=restore-and-step-one-frame"
    ) {
        mapper_1_rom_harness().run_frame();
    };

    BENCHMARK(
        "ppu mapper=2 rom=mega-man render-mode=full-frame "
        "operation=restore-and-step-one-frame"
    ) {
        mapper_2_rom_harness().run_frame();
    };

    BENCHMARK(
        "ppu mapper=3 rom=adventure-island render-mode=full-frame "
        "operation=restore-and-step-one-frame"
    ) {
        mapper_3_rom_harness().run_frame();
    };
}
