//  Program:      nes-py
//  File:         test_mapper_hooks.cpp
//  Description:  Catch2 coverage for mapper lifecycle and hook routing
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include <catch2/catch_test_macros.hpp>
#include "nes_emu/test/nes_emu/support/test_mappers.hpp"

namespace {

bool mapper_irq_reaches_cpu_irq_vector() {
    NESTest::IRQTestMapper mapper;
    NES::MainBus bus;
    NES::CPU cpu;
    bus.set_mapper(&mapper);
    mapper.setIRQCallback([&]() {
        cpu.interrupt(bus, NES::CPU::IRQ_INTERRUPT);
    });
    cpu.reset(bus);
    for (int index = 0; index < 4; ++index)
        cpu.cycle(bus);
    mapper.triggerIRQ();
    for (int index = 0; index < 32; ++index)
        cpu.cycle(bus);
    return bus.read(0x0002) == 0x42;
}

bool mapper_cpu_cycle_hooks_are_observable() {
    NESTest::CPUCycleHookMapper mapper;
    mapper.onCPUCycle();
    mapper.onCPUCycle();
    mapper.onCPUCycle();
    return mapper.cpu_cycles == 3;
}

bool mapper_ppu_hooks_are_observable() {
    NESTest::PPUHookMapper mapper;
    NES::PictureBus bus;
    bus.set_mapper(&mapper);
    NES::NES_Byte value = bus.read(0x0123);
    bus.write(0x0456, 0x77);
    return (
        value == 0x33 &&
        mapper.address_observations == 2 &&
        mapper.read_observations == 1 &&
        mapper.write_observations == 1 &&
        mapper.last_address == 0x0456 &&
        mapper.last_value == 0x77
    );
}

bool mapper_expansion_area_routes_through_main_bus() {
    NESTest::ExpansionTestMapper mapper;
    NES::MainBus bus;
    bus.set_mapper(&mapper);
    bus.write(0x5000, 0x5a);
    return bus.read(0x5000) == 0x5a;
}

bool mapper_prg_ram_banking_and_protection_are_observable() {
    NESTest::PRGRAMTestMapper mapper;
    NES::MainBus bus;
    bus.set_mapper(&mapper);
    bus.write(0x6000, 0x11);
    bus.write(0x8000, 0x01);
    bus.write(0x6000, 0x22);
    bool banked = bus.read(0x6000) == 0x22;
    bus.write(0x8000, 0x00);
    banked = banked && bus.read(0x6000) == 0x11;
    mapper.protectPRGRAM();
    bus.write(0x6000, 0x99);
    return banked && bus.read(0x6000) == 0x11;
}

bool mapper_name_table_mapping_routes_through_picture_bus() {
    NESTest::NameTableTestMapper mapper;
    NES::PictureBus bus;
    bus.set_mapper(&mapper);
    bus.write(0x2401, 0x66);
    return bus.read(0x2401) == 0x66;
}

}  // namespace

TEST_CASE(
    "mapper lifecycle and timing extension points route through native buses",
    "[mapper]"
) {
    REQUIRE(mapper_irq_reaches_cpu_irq_vector());
    REQUIRE(mapper_cpu_cycle_hooks_are_observable());
    REQUIRE(mapper_ppu_hooks_are_observable());
    REQUIRE(mapper_expansion_area_routes_through_main_bus());
    REQUIRE(mapper_prg_ram_banking_and_protection_are_observable());
    REQUIRE(mapper_name_table_mapping_routes_through_picture_bus());
}
