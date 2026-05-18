//  Program:      nes-py
//  File:         test_main_bus.cpp
//  Description:  Catch2 main-bus characterization tests
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include <catch2/catch_test_macros.hpp>
#include "nes_emu/test/nes_emu/support/test_mappers.hpp"

namespace {

bool ram_is_mirrored_every_2k() {
    NES::MainBus bus;
    bus.write(0x0002, 0x12);
    return (
        bus.read(0x0002) == 0x12 &&
        bus.read(0x0802) == 0x12 &&
        bus.read(0x1002) == 0x12 &&
        bus.read(0x1802) == 0x12
    );
}

bool ppu_register_callbacks_remain_available_without_direct_devices() {
    NES::MainBus bus;
    NES::NES_Byte written = 0;
    int writes = 0;
    bus.set_write_callback(NES::PPUCTRL, [&](NES::NES_Byte value) {
        written = value;
        ++writes;
    });
    bus.set_read_callback(NES::PPUSTATUS, [&]() {
        return static_cast<NES::NES_Byte>(0xa5);
    });
    bus.write(0x2008, 0x5c);
    return writes == 1 && written == 0x5c && bus.read(0x200a) == 0xa5;
}

bool controller_reads_shift_serial_button_state() {
    NES::MainBus bus;
    NES::Controller controllers[2];
    controllers[0].write_buttons(0x05);
    bus.connect_devices(nullptr, nullptr, nullptr, controllers);
    bus.write(NES::JOY1, 0x00);
    NES::NES_Byte first = bus.read(NES::JOY1);
    NES::NES_Byte second = bus.read(NES::JOY1);
    NES::NES_Byte third = bus.read(NES::JOY1);
    return first == 0x41 && second == 0x40 && third == 0x41;
}

bool oam_dma_reads_from_selected_cpu_page() {
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
    bus.connect_devices(&cpu, &ppu, &picture_bus, controllers);
    bus.write(0x0300, 0x6d);
    bus.write(NES::OAMADDR, 0x00);
    bus.write(NES::OAMDMA, 0x03);
    bus.write(NES::OAMADDR, 0x00);
    return bus.read(NES::OAMDATA) == 0x6d;
}

bool expansion_area_uses_mapper_when_present() {
    NESTest::ExpansionTestMapper mapper;
    NES::MainBus mapped_bus;
    NES::MainBus default_bus;
    mapped_bus.set_mapper(&mapper);
    mapped_bus.write(0x5000, 0x5a);
    return mapped_bus.read(0x5000) == 0x5a && default_bus.read(0x5000) == 0x00;
}

bool prg_ram_routes_through_mapper() {
    NESTest::PRGRAMTestMapper mapper;
    NES::MainBus bus;
    bus.set_mapper(&mapper);
    bus.write(0x6001, 0x27);
    return bus.read(0x6001) == 0x27;
}

bool prg_rom_routes_through_mapper() {
    NESTest::ProgramTestMapper mapper;
    NES::MainBus bus;
    mapper.setByte(0x8123, 0x9e);
    bus.set_mapper(&mapper);
    bus.write(0x9000, 0x44);
    return (
        bus.read(0x8123) == 0x9e &&
        mapper.lastPRGWriteAddress() == 0x9000 &&
        mapper.lastPRGWriteValue() == 0x44
    );
}

bool prg_rom_prefers_direct_read_pages_and_refreshes_after_writes() {
    NESTest::DirectReadTestMapper mapper;
    NES::MainBus bus;
    bus.set_mapper(&mapper);

    bool initial_direct = (
        bus.read(0x8000) == 0x80 &&
        bus.read(0xa000) == 0x81 &&
        bus.read(0xc000) == 0x82 &&
        bus.read(0xe000) == 0x83
    );

    bus.write(0x9000, 0x5a);
    return initial_direct && bus.read(0x8000) == 0x5a;
}

}  // namespace

TEST_CASE("main bus mirrors RAM and routes devices through fixed paths", "[bus]") {
    REQUIRE(ram_is_mirrored_every_2k());
    REQUIRE(ppu_register_callbacks_remain_available_without_direct_devices());
    REQUIRE(controller_reads_shift_serial_button_state());
    REQUIRE(oam_dma_reads_from_selected_cpu_page());
    REQUIRE(expansion_area_uses_mapper_when_present());
    REQUIRE(prg_ram_routes_through_mapper());
    REQUIRE(prg_rom_routes_through_mapper());
    REQUIRE(prg_rom_prefers_direct_read_pages_and_refreshes_after_writes());
}
