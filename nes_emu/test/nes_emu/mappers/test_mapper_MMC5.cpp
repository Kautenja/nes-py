//  Program:      nes-py
//  File:         test_mapper_MMC5.cpp
//  Description:  Catch2 coverage for native mapper 005 / MMC5
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include <memory>
#include <catch2/catch_test_macros.hpp>
#include "nes_emu/test/nes_emu/support/emulator_inspector.hpp"
#include "nes_emu/test/nes_emu/support/mapper_test_helpers.hpp"
#include "nes_emu/test/nes_emu/support/synthetic_rom.hpp"

namespace {

void enable_prg_ram_writes(NES::Mapper& mapper) {
    mapper.writeExpansion(0x5102, 0x02);
    mapper.writeExpansion(0x5103, 0x01);
}

void run_mmc5_scanline(NES::Mapper& mapper) {
    for (NES::NES_Address tile = 0; tile < 32; ++tile)
        mapper.readNameTable(0x2000 + tile);
}

}  // namespace

TEST_CASE("mapper 005 MMC5 is registered by the native factory", "[mapper][mmc5]") {
    REQUIRE(NES::IsMapperSupported(5));

    NESTest::TemporaryROM rom(
        "mmc5_factory",
        5,
        8,
        8,
        "horizontal",
        false,
        true,
        true
    );
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NES::MapperFactory(&cartridge);
    REQUIRE(mapper != nullptr);
    REQUIRE(mapper->readPRG(0xe010) == NESTest::prg_8k_bank_marker(15));
}

TEST_CASE("mapper 005 MMC5 supports PRG banking modes", "[mapper][mmc5]") {
    NESTest::TemporaryROM rom(
        "mmc5_prg",
        5,
        8,
        8,
        "horizontal",
        false,
        true,
        true
    );
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    mapper->writeExpansion(0x5100, 0x03);
    mapper->writeExpansion(0x5114, 0x82);
    mapper->writeExpansion(0x5115, 0x83);
    mapper->writeExpansion(0x5116, 0x84);
    mapper->writeExpansion(0x5117, 0x85);
    REQUIRE(mapper->readPRG(0x8010) == NESTest::prg_8k_bank_marker(2));
    REQUIRE(mapper->readPRG(0xa010) == NESTest::prg_8k_bank_marker(3));
    REQUIRE(mapper->readPRG(0xc010) == NESTest::prg_8k_bank_marker(4));
    REQUIRE(mapper->readPRG(0xe010) == NESTest::prg_8k_bank_marker(5));

    mapper->writeExpansion(0x5100, 0x02);
    mapper->writeExpansion(0x5115, 0x82);
    mapper->writeExpansion(0x5116, 0x86);
    mapper->writeExpansion(0x5117, 0x8f);
    REQUIRE(mapper->readPRG(0x8010) == NESTest::prg_8k_bank_marker(2));
    REQUIRE(mapper->readPRG(0xa010) == NESTest::prg_8k_bank_marker(3));
    REQUIRE(mapper->readPRG(0xc010) == NESTest::prg_8k_bank_marker(6));
    REQUIRE(mapper->readPRG(0xe010) == NESTest::prg_8k_bank_marker(15));

    mapper->writeExpansion(0x5100, 0x01);
    mapper->writeExpansion(0x5115, 0x84);
    mapper->writeExpansion(0x5117, 0x8c);
    REQUIRE(mapper->readPRG(0x8010) == NESTest::prg_8k_bank_marker(4));
    REQUIRE(mapper->readPRG(0xa010) == NESTest::prg_8k_bank_marker(5));
    REQUIRE(mapper->readPRG(0xc010) == NESTest::prg_8k_bank_marker(12));
    REQUIRE(mapper->readPRG(0xe010) == NESTest::prg_8k_bank_marker(13));

    mapper->writeExpansion(0x5100, 0x00);
    mapper->writeExpansion(0x5117, 0x88);
    REQUIRE(mapper->readPRG(0x8010) == NESTest::prg_8k_bank_marker(8));
    REQUIRE(mapper->readPRG(0xa010) == NESTest::prg_8k_bank_marker(9));
    REQUIRE(mapper->readPRG(0xc010) == NESTest::prg_8k_bank_marker(10));
    REQUIRE(mapper->readPRG(0xe010) == NESTest::prg_8k_bank_marker(11));
}

TEST_CASE("mapper 005 MMC5 banks and protects PRG RAM", "[mapper][mmc5]") {
    NESTest::TemporaryROM rom("mmc5_wram", 5, 4, 0);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    mapper->writePRGRAM(0x6000, 0x11);
    REQUIRE(mapper->readPRGRAM(0x6000) == 0x00);

    enable_prg_ram_writes(*mapper);
    mapper->writeExpansion(0x5113, 0x00);
    mapper->writePRGRAM(0x6000, 0x11);
    mapper->writeExpansion(0x5113, 0x01);
    mapper->writePRGRAM(0x6000, 0x22);
    REQUIRE(mapper->readPRGRAM(0x6000) == 0x22);
    mapper->writeExpansion(0x5113, 0x00);
    REQUIRE(mapper->readPRGRAM(0x6000) == 0x11);

    mapper->writeExpansion(0x5100, 0x03);
    mapper->writeExpansion(0x5114, 0x02);
    mapper->writePRG(0x8000, 0x33);
    REQUIRE(mapper->readPRG(0x8000) == 0x33);
    mapper->writeExpansion(0x5114, 0x82);
    REQUIRE(mapper->readPRG(0x8000) == NESTest::prg_bank_marker(1));

    mapper->writeExpansion(0x5103, 0x00);
    mapper->writeExpansion(0x5113, 0x00);
    mapper->writePRGRAM(0x6000, 0x99);
    REQUIRE(mapper->readPRGRAM(0x6000) == 0x11);
}

TEST_CASE("mapper 005 MMC5 supports CHR modes and background banks", "[mapper][mmc5]") {
    NESTest::TemporaryROM rom(
        "mmc5_chr",
        5,
        4,
        4,
        "horizontal",
        false,
        false,
        true
    );
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    mapper->writeExpansion(0x5101, 0x03);
    mapper->writeExpansion(0x5120, 0x01);
    mapper->writeExpansion(0x5121, 0x02);
    mapper->writeExpansion(0x5127, 0x07);
    REQUIRE(mapper->readCHR(0x0000) == NESTest::chr_1k_page_marker(1));
    REQUIRE(mapper->readCHR(0x0400) == NESTest::chr_1k_page_marker(2));
    REQUIRE(mapper->readCHR(0x1c00) == NESTest::chr_1k_page_marker(7));

    mapper->writeExpansion(0x5101, 0x02);
    mapper->writeExpansion(0x5121, 0x03);
    REQUIRE(mapper->readCHR(0x0000) == NESTest::chr_1k_page_marker(6));
    REQUIRE(mapper->readCHR(0x0400) == NESTest::chr_1k_page_marker(7));

    mapper->writeExpansion(0x5101, 0x01);
    mapper->writeExpansion(0x5123, 0x02);
    REQUIRE(mapper->readCHR(0x0000) == NESTest::chr_1k_page_marker(8));
    REQUIRE(mapper->readCHR(0x0c00) == NESTest::chr_1k_page_marker(11));

    mapper->writeExpansion(0x5101, 0x03);
    mapper->writeExpansion(0x5120, 0x01);
    mapper->writeExpansion(0x5128, 0x05);
    mapper->readNameTable(0x2000);
    REQUIRE(mapper->readCHR(0x0000) == NESTest::chr_1k_page_marker(5));
    REQUIRE(mapper->readCHR(0x0000) == NESTest::chr_1k_page_marker(5));
    REQUIRE(mapper->readCHR(0x0000) == NESTest::chr_1k_page_marker(1));
}

TEST_CASE("mapper 005 MMC5 maps ExRAM, nametables, and fill mode", "[mapper][mmc5]") {
    NESTest::TemporaryROM rom(
        "mmc5_exram",
        5,
        4,
        4,
        "horizontal",
        false,
        false,
        true
    );
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    mapper->writeExpansion(0x5104, 0x02);
    mapper->writeExpansion(0x5c00, 0x5a);
    REQUIRE(mapper->readExpansion(0x5c00) == 0x5a);
    mapper->writeExpansion(0x5104, 0x03);
    mapper->writeExpansion(0x5c00, 0x99);
    REQUIRE(mapper->readExpansion(0x5c00) == 0x5a);
    mapper->writeExpansion(0x5104, 0x00);
    REQUIRE(mapper->readExpansion(0x5c00) == 0x00);

    mapper->writeExpansion(0x5105, 0xe4);
    mapper->writeNameTable(0x2001, 0x11);
    mapper->writeNameTable(0x2401, 0x22);
    mapper->writeNameTable(0x2802, 0x33);
    REQUIRE(mapper->readNameTable(0x2001) == 0x11);
    REQUIRE(mapper->readNameTable(0x2401) == 0x22);
    REQUIRE(mapper->readNameTable(0x2802) == 0x33);

    mapper->writeExpansion(0x5106, 0x44);
    mapper->writeExpansion(0x5107, 0x02);
    REQUIRE(mapper->readNameTable(0x2c02) == 0x44);
    REQUIRE(mapper->readNameTable(0x2fc0) == 0xaa);

    mapper->writeExpansion(0x5104, 0x01);
    mapper->writeExpansion(0x5105, 0x00);
    mapper->writeExpansion(0x5c03, 0xc5);
    mapper->writeExpansion(0x5101, 0x03);
    mapper->readNameTable(0x2003);
    REQUIRE(mapper->readNameTable(0x23c0) == 0xff);
    REQUIRE(mapper->readCHR(0x0000) == NESTest::chr_1k_page_marker(20));
}

TEST_CASE("mapper 005 MMC5 exposes IRQ status and multiplier registers", "[mapper][mmc5]") {
    NESTest::TemporaryROM rom("mmc5_irq", 5, 4, 0);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);
    int irq_count = 0;
    mapper->setIRQCallback([&]() { ++irq_count; });

    REQUIRE(mapper->readExpansion(0x5205) == 0x01);
    REQUIRE(mapper->readExpansion(0x5206) == 0xfe);
    mapper->writeExpansion(0x5205, 0x13);
    mapper->writeExpansion(0x5206, 0x11);
    REQUIRE(mapper->readExpansion(0x5205) == 0x43);
    REQUIRE(mapper->readExpansion(0x5206) == 0x01);

    mapper->writeExpansion(0x5203, 0x02);
    mapper->writeExpansion(0x5204, 0x80);
    run_mmc5_scanline(*mapper);
    run_mmc5_scanline(*mapper);
    run_mmc5_scanline(*mapper);
    REQUIRE(irq_count == 1);
    REQUIRE(mapper->readExpansion(0x5204) == 0xc0);
    REQUIRE(mapper->readExpansion(0x5204) == 0x40);

    mapper->onCPUCycle();
    mapper->onCPUCycle();
    mapper->onCPUCycle();
    mapper->onCPUCycle();
    REQUIRE((mapper->readExpansion(0x5204) & 0x40) == 0);
}

TEST_CASE("mapper 005 emulator save-state preserves MMC5 state", "[mapper][mmc5]") {
    NESTest::TemporaryROM rom(
        "mmc5_emulator_backup",
        5,
        8,
        8,
        "horizontal",
        false,
        true,
        true
    );
    NES::Emulator emulator(rom.filename());
    NES::Mapper& mapper = NES::EmulatorInspector::mapper(emulator);

    enable_prg_ram_writes(mapper);
    mapper.writeExpansion(0x5100, 0x03);
    mapper.writeExpansion(0x5113, 0x00);
    mapper.writeExpansion(0x5114, 0x82);
    mapper.writePRGRAM(0x6000, 0x5a);
    mapper.writeExpansion(0x5105, 0xff);
    mapper.writeExpansion(0x5106, 0x44);
    mapper.writeExpansion(0x5107, 0x03);
    mapper.writeExpansion(0x5205, 0x12);
    mapper.writeExpansion(0x5206, 0x10);
    emulator.backup();

    mapper.writeExpansion(0x5114, 0x83);
    mapper.writePRGRAM(0x6000, 0xa5);
    mapper.writeExpansion(0x5106, 0x55);
    mapper.writeExpansion(0x5205, 0x01);

    emulator.restore();
    NES::Mapper& restored = NES::EmulatorInspector::mapper(emulator);
    REQUIRE(restored.readPRG(0x8000) == NESTest::prg_8k_bank_marker(2));
    REQUIRE(restored.readPRGRAM(0x6000) == 0x5a);
    REQUIRE(restored.readNameTable(0x2000) == 0x44);
    REQUIRE(restored.readNameTable(0x23c0) == 0xff);
    REQUIRE(restored.readExpansion(0x5205) == 0x20);
    REQUIRE(restored.readExpansion(0x5206) == 0x01);
}
