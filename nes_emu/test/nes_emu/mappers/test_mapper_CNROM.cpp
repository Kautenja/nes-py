//  Program:      nes-py
//  File:         test_mapper_CNROM.cpp
//  Description:  Catch2 coverage for native mapper 003 / CNROM
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include <memory>
#include <catch2/catch_test_macros.hpp>
#include "nes_emu/test/nes_emu/support/emulator_inspector.hpp"
#include "nes_emu/test/nes_emu/support/mapper_test_helpers.hpp"
#include "nes_emu/test/nes_emu/support/synthetic_rom.hpp"

TEST_CASE("mapper 003 CNROM switches CHR banks and masks selects", "[mapper][cnrom]") {
    NESTest::TemporaryROM rom("cnrom", 3, 2, 4);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(0));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(1));
    REQUIRE(mapper->readCHR(0x0100) == NESTest::chr_bank_marker(0));
    REQUIRE(NESTest::direct_prg_read(*mapper, 0x9000) == NESTest::prg_bank_marker(0));
    REQUIRE(NESTest::direct_prg_read(*mapper, 0xd000) == NESTest::prg_bank_marker(1));
    REQUIRE(NESTest::direct_chr_read(*mapper, 0x0100) == NESTest::chr_bank_marker(0));

    mapper->writePRG(0x8000, 0x02);
    REQUIRE(mapper->readCHR(0x0100) == NESTest::chr_bank_marker(2));
    REQUIRE(NESTest::direct_chr_read(*mapper, 0x0100) == NESTest::chr_bank_marker(2));
    mapper->writePRG(0x8000, 0x03);
    REQUIRE(mapper->readCHR(0x0100) == NESTest::chr_bank_marker(3));
    REQUIRE(NESTest::direct_chr_read(*mapper, 0x0100) == NESTest::chr_bank_marker(3));
}

TEST_CASE("mapper 003 masks CHR bank selects to available banks", "[mapper][cnrom]") {
    NESTest::TemporaryROM rom("cnrom_masked", 3, 2, 2);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    mapper->writePRG(0x8000, 0x03);
    REQUIRE(mapper->readCHR(0x0100) == NESTest::chr_bank_marker(1));
    REQUIRE(NESTest::direct_chr_read(*mapper, 0x0100) == NESTest::chr_bank_marker(1));
}

TEST_CASE("mapper 003 emulator save-state preserves selected CHR bank", "[mapper][cnrom]") {
    NESTest::TemporaryROM rom("cnrom_emulator_backup", 3, 2, 4);
    NES::Emulator emulator(rom.filename());
    NES::Mapper& mapper = NES::EmulatorInspector::mapper(emulator);

    REQUIRE(mapper.readPRG(0x9000) == NESTest::prg_bank_marker(0));
    REQUIRE(mapper.readPRG(0xd000) == NESTest::prg_bank_marker(1));
    REQUIRE(mapper.readCHR(0x0100) == NESTest::chr_bank_marker(0));

    mapper.writePRG(0x8000, 0x02);
    REQUIRE(mapper.readCHR(0x0100) == NESTest::chr_bank_marker(2));

    emulator.backup();

    mapper.writePRG(0x8000, 0x03);
    REQUIRE(mapper.readCHR(0x0100) == NESTest::chr_bank_marker(3));

    emulator.restore();
    NES::Mapper& restored = NES::EmulatorInspector::mapper(emulator);
    REQUIRE(restored.readPRG(0x9000) == NESTest::prg_bank_marker(0));
    REQUIRE(restored.readPRG(0xd000) == NESTest::prg_bank_marker(1));
    REQUIRE(restored.readCHR(0x0100) == NESTest::chr_bank_marker(2));
    REQUIRE(NESTest::direct_prg_read(restored, 0x9000) == NESTest::prg_bank_marker(0));
    REQUIRE(NESTest::direct_prg_read(restored, 0xd000) == NESTest::prg_bank_marker(1));
    REQUIRE(NESTest::direct_chr_read(restored, 0x0100) == NESTest::chr_bank_marker(2));
}
