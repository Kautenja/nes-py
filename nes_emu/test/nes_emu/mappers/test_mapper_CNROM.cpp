//  Program:      nes-py
//  File:         test_mapper_CNROM.cpp
//  Description:  Catch2 coverage for native mapper 003 / CNROM
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include <memory>
#include <catch2/catch_test_macros.hpp>
#include "nes_emu/test/nes_emu/support/mapper_test_helpers.hpp"
#include "nes_emu/test/nes_emu/support/synthetic_rom.hpp"

TEST_CASE("mapper 003 CNROM switches CHR banks and masks selects", "[mapper][cnrom]") {
    NESTest::TemporaryROM rom("cnrom", 3, 2, 4);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(0));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(1));
    REQUIRE(mapper->readCHR(0x0100) == NESTest::chr_bank_marker(0));

    mapper->writePRG(0x8000, 0x02);
    REQUIRE(mapper->readCHR(0x0100) == NESTest::chr_bank_marker(2));
    mapper->writePRG(0x8000, 0x03);
    REQUIRE(mapper->readCHR(0x0100) == NESTest::chr_bank_marker(3));
}

TEST_CASE("mapper 003 masks CHR bank selects to available banks", "[mapper][cnrom]") {
    NESTest::TemporaryROM rom("cnrom_masked", 3, 2, 2);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    mapper->writePRG(0x8000, 0x03);
    REQUIRE(mapper->readCHR(0x0100) == NESTest::chr_bank_marker(1));
}
