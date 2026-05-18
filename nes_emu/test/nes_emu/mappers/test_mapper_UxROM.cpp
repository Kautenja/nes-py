//  Program:      nes-py
//  File:         test_mapper_UxROM.cpp
//  Description:  Catch2 coverage for native mapper 002 / UxROM
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include <memory>
#include <catch2/catch_test_macros.hpp>
#include "nes_emu/test/nes_emu/support/emulator_inspector.hpp"
#include "nes_emu/test/nes_emu/support/mapper_test_helpers.hpp"
#include "nes_emu/test/nes_emu/support/synthetic_rom.hpp"

TEST_CASE("mapper 002 UxROM switches PRG and provides CHR RAM", "[mapper][uxrom]") {
    NESTest::TemporaryROM rom("uxrom", 2, 4, 0);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(0));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(3));
    REQUIRE(NESTest::direct_prg_read(*mapper, 0x9000) == NESTest::prg_bank_marker(0));
    REQUIRE(NESTest::direct_prg_read(*mapper, 0xd000) == NESTest::prg_bank_marker(3));

    mapper->writePRG(0x8000, 0x02);
    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(2));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(3));
    REQUIRE(NESTest::direct_prg_read(*mapper, 0x9000) == NESTest::prg_bank_marker(2));
    REQUIRE(NESTest::direct_prg_read(*mapper, 0xd000) == NESTest::prg_bank_marker(3));

    mapper->writeCHR(0x0456, 0x3c);
    REQUIRE(mapper->readCHR(0x0456) == 0x3c);
    REQUIRE(NESTest::direct_chr_read(*mapper, 0x0456) == 0x3c);

    mapper->writePRG(0x8000, 0x05);
    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(1));
    REQUIRE(NESTest::direct_prg_read(*mapper, 0x9000) == NESTest::prg_bank_marker(1));
}

TEST_CASE(
    "mapper 002 emulator save-state preserves selected PRG bank and CHR RAM",
    "[mapper][uxrom]"
) {
    NESTest::TemporaryROM rom("uxrom_emulator_backup", 2, 4, 0);
    NES::Emulator emulator(rom.filename());
    NES::Mapper& mapper = NES::EmulatorInspector::mapper(emulator);

    mapper.writePRG(0x8000, 0x02);
    mapper.writeCHR(0x0456, 0x3c);
    REQUIRE(mapper.readPRG(0x9000) == NESTest::prg_bank_marker(2));
    REQUIRE(mapper.readPRG(0xd000) == NESTest::prg_bank_marker(3));
    REQUIRE(mapper.readCHR(0x0456) == 0x3c);

    emulator.backup();

    mapper.writePRG(0x8000, 0x01);
    mapper.writeCHR(0x0456, 0xa5);
    REQUIRE(mapper.readPRG(0x9000) == NESTest::prg_bank_marker(1));
    REQUIRE(mapper.readPRG(0xd000) == NESTest::prg_bank_marker(3));
    REQUIRE(mapper.readCHR(0x0456) == 0xa5);

    emulator.restore();
    NES::Mapper& restored = NES::EmulatorInspector::mapper(emulator);
    REQUIRE(restored.readPRG(0x9000) == NESTest::prg_bank_marker(2));
    REQUIRE(restored.readPRG(0xd000) == NESTest::prg_bank_marker(3));
    REQUIRE(restored.readCHR(0x0456) == 0x3c);
    REQUIRE(NESTest::direct_prg_read(restored, 0x9000) == NESTest::prg_bank_marker(2));
    REQUIRE(NESTest::direct_prg_read(restored, 0xd000) == NESTest::prg_bank_marker(3));
    REQUIRE(NESTest::direct_chr_read(restored, 0x0456) == 0x3c);
}
