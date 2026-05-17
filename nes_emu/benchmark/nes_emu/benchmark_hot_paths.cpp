//  Program:      nes-py
//  File:         benchmark_hot_paths.cpp
//  Description:  Catch2 native hot-path benchmarks
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include "nes_emu/test/nes_emu/support/test_mappers.hpp"

namespace {

volatile NES::NES_Byte benchmark_sink = 0;

void cpu_dispatch_work(int iterations) {
    NESTest::ProgramTestMapper mapper;
    NES::MainBus bus;
    NES::CPU cpu;
    mapper.load(0x8000, {
        0xa9, 0x01,       // LDA #$01
        0xaa,             // TAX
        0xe8,             // INX
        0x85, 0x00,       // STA $00
        0xa2, 0x02,       // LDX #$02
        0xc0, 0x02,       // CPY #$02
        0x4c, 0x00, 0x80  // JMP $8000
    });
    bus.set_mapper(&mapper);
    cpu.reset(bus);
    NESTest::run_cpu_cycles(cpu, bus, iterations);
    benchmark_sink ^= bus.read(0x0000);
}

void main_bus_io_dispatch_work(int iterations) {
    NESTest::ProgramTestMapper mapper;
    NES::MainBus bus;
    NES::PictureBus picture_bus;
    NES::PPU ppu;
    NES::CPU cpu;
    NES::Controller controllers[2];
    bus.set_mapper(&mapper);
    picture_bus.set_mapper(&mapper);
    ppu.reset();
    cpu.reset(bus);
    controllers[0].write_buttons(0x01);
    bus.connect_devices(&cpu, &ppu, &picture_bus, controllers);
    for (int index = 0; index < iterations; ++index) {
        NES::NES_Byte value = static_cast<NES::NES_Byte>(index);
        bus.write(0x0000, value);
        benchmark_sink ^= bus.read(0x0800);
        bus.write(NES::OAMADDR, 0x00);
        bus.write(NES::OAMDATA, value);
        benchmark_sink ^= bus.read(NES::OAMDATA);
        bus.write(NES::JOY1, 0x00);
        benchmark_sink ^= bus.read(NES::JOY1);
        bus.write(0x6000, value);
        benchmark_sink ^= bus.read(0x6000);
        bus.write(0x9000, value);
        benchmark_sink ^= bus.read(0x8000);
    }
}

void mapper_cycle_work(int iterations, bool hooked) {
    NESTest::CPUCycleHookMapper hooked_mapper;
    NESTest::ProgramTestMapper unhooked_mapper;
    NES::Mapper* mapper = hooked ?
        static_cast<NES::Mapper*>(&hooked_mapper) :
        static_cast<NES::Mapper*>(&unhooked_mapper);
    volatile bool observes = mapper->observesCPUCycles();
    for (int index = 0; index < iterations; ++index) {
        if (observes)
            mapper->onCPUCycle();
    }
    if (hooked)
        benchmark_sink ^= static_cast<NES::NES_Byte>(hooked_mapper.cpu_cycles);
}

}  // namespace

TEST_CASE("native CPU, bus, and mapper hook hot paths", "[benchmark]") {
    BENCHMARK("cpu dispatch") {
        cpu_dispatch_work(250);
    };

    BENCHMARK("main bus I/O dispatch") {
        main_bus_io_dispatch_work(250);
    };

    BENCHMARK("mapper CPU-cycle dispatch without hook") {
        mapper_cycle_work(1000, false);
    };

    BENCHMARK("mapper CPU-cycle dispatch with hook") {
        mapper_cycle_work(1000, true);
    };
}
