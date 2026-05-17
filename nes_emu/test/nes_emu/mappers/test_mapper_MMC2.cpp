//  Program:      nes-py
//  File:         test_mapper_MMC2.cpp
//  Description:  Catch2 coverage for native mapper 009 / MMC2
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include <memory>
#include <catch2/catch_test_macros.hpp>
#include "nes_emu/test/nes_emu/support/emulator_inspector.hpp"
#include "nes_emu/test/nes_emu/support/mapper_test_helpers.hpp"
#include "nes_emu/test/nes_emu/support/synthetic_rom.hpp"

TEST_CASE(
    "mapper 009 MMC2 switches PRG banks and mirroring",
    "[mapper][mmc2]"
) {
    NESTest::TemporaryROM rom(
        "mmc2_prg",
        9,
        4,
        4,
        "horizontal",
        true,
        true
    );
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    REQUIRE(NES::IsMapperSupported(9));
    REQUIRE(mapper->observesPPUAddresses());
    REQUIRE_FALSE(mapper->observesCPUCycles());
    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_page_marker(0));
    REQUIRE(mapper->readPRG(0xb000) == NESTest::prg_page_marker(5));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_page_marker(6));
    REQUIRE(mapper->readPRG(0xf000) == NESTest::prg_page_marker(7));

    mapper->writePRG(0xa123, 0x1b);
    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_page_marker(3));

    mapper->writePRG(0xffff, 0x00);
    REQUIRE(mapper->getNameTableMirroring() == NES::VERTICAL);
    mapper->writePRG(0xf000, 0x01);
    REQUIRE(mapper->getNameTableMirroring() == NES::HORIZONTAL);
}

TEST_CASE(
    "mapper 009 MMC2 latches exact CHR banks from PPU addresses",
    "[mapper][mmc2]"
) {
    NESTest::TemporaryROM rom(
        "mmc2_chr_latches",
        9,
        4,
        8,
        "horizontal",
        true
    );
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);
    auto ppu_read = [&](NES::NES_Address address) {
        mapper->onPPUAddress(address);
        return mapper->readCHR(address);
    };

    mapper->writePRG(0xb000, 0x21);
    mapper->writePRG(0xc000, 0x22);
    mapper->writePRG(0xd000, 0x23);
    mapper->writePRG(0xe000, 0x24);

    REQUIRE(mapper->readCHR(0x0000) == NESTest::chr_page_marker(2));
    REQUIRE(mapper->readCHR(0x1000) == NESTest::chr_page_marker(4));

    ppu_read(0x0fd7);
    REQUIRE(mapper->readCHR(0x0000) == NESTest::chr_page_marker(2));
    ppu_read(0x0fd9);
    REQUIRE(mapper->readCHR(0x0000) == NESTest::chr_page_marker(2));
    REQUIRE(ppu_read(0x0fd8) == NESTest::chr_page_marker(2));
    REQUIRE(mapper->readCHR(0x0000) == NESTest::chr_page_marker(1));
    ppu_read(0x0fe7);
    REQUIRE(mapper->readCHR(0x0000) == NESTest::chr_page_marker(1));
    ppu_read(0x0fe9);
    REQUIRE(mapper->readCHR(0x0000) == NESTest::chr_page_marker(1));
    REQUIRE(ppu_read(0x0fe8) == NESTest::chr_page_marker(1));
    REQUIRE(mapper->readCHR(0x0000) == NESTest::chr_page_marker(2));

    ppu_read(0x1fd7);
    REQUIRE(mapper->readCHR(0x1000) == NESTest::chr_page_marker(4));
    for (NES::NES_Address address = 0x1fd8; address <= 0x1fdf; ++address) {
        ppu_read(0x1fe8);
        REQUIRE(mapper->readCHR(0x1000) == NESTest::chr_page_marker(4));
        REQUIRE(ppu_read(address) == NESTest::chr_page_marker(4));
        REQUIRE(mapper->readCHR(0x1000) == NESTest::chr_page_marker(3));
    }
    ppu_read(0x1fe7);
    REQUIRE(mapper->readCHR(0x1000) == NESTest::chr_page_marker(3));
    ppu_read(0x1ff0);
    REQUIRE(mapper->readCHR(0x1000) == NESTest::chr_page_marker(3));
    for (NES::NES_Address address = 0x1fe8; address <= 0x1fef; ++address) {
        ppu_read(0x1fd8);
        REQUIRE(mapper->readCHR(0x1000) == NESTest::chr_page_marker(3));
        REQUIRE(ppu_read(address) == NESTest::chr_page_marker(3));
        REQUIRE(mapper->readCHR(0x1000) == NESTest::chr_page_marker(4));
    }
}

TEST_CASE(
    "mapper 009 emulator save-state preserves MMC2 state",
    "[mapper][mmc2]"
) {
    NESTest::TemporaryROM rom(
        "mmc2_emulator_backup",
        9,
        4,
        8,
        "horizontal",
        true,
        true
    );
    NES::Emulator emulator(rom.filename());
    NES::Mapper& mapper = NES::EmulatorInspector::mapper(emulator);

    mapper.writePRG(0xa000, 0x03);
    mapper.writePRG(0xb000, 0x01);
    mapper.writePRG(0xc000, 0x02);
    mapper.writePRG(0xd000, 0x03);
    mapper.writePRG(0xe000, 0x04);
    mapper.writePRG(0xf000, 0x00);
    mapper.onPPUAddress(0x0fd8);
    mapper.readCHR(0x0fd8);
    mapper.onPPUAddress(0x1fe8);
    mapper.readCHR(0x1fe8);

    REQUIRE(mapper.readPRG(0x9000) == NESTest::prg_page_marker(3));
    REQUIRE(mapper.readCHR(0x0000) == NESTest::chr_page_marker(1));
    REQUIRE(mapper.readCHR(0x1000) == NESTest::chr_page_marker(4));
    REQUIRE(mapper.getNameTableMirroring() == NES::VERTICAL);

    emulator.backup();

    mapper.writePRG(0xa000, 0x04);
    mapper.writePRG(0xf000, 0x01);
    mapper.onPPUAddress(0x0fe8);
    mapper.readCHR(0x0fe8);
    mapper.onPPUAddress(0x1fd8);
    mapper.readCHR(0x1fd8);
    REQUIRE(mapper.readPRG(0x9000) == NESTest::prg_page_marker(4));
    REQUIRE(mapper.readCHR(0x0000) == NESTest::chr_page_marker(2));
    REQUIRE(mapper.readCHR(0x1000) == NESTest::chr_page_marker(3));
    REQUIRE(mapper.getNameTableMirroring() == NES::HORIZONTAL);

    emulator.restore();
    NES::Mapper& restored = NES::EmulatorInspector::mapper(emulator);
    REQUIRE(restored.readPRG(0x9000) == NESTest::prg_page_marker(3));
    REQUIRE(restored.readCHR(0x0000) == NESTest::chr_page_marker(1));
    REQUIRE(restored.readCHR(0x1000) == NESTest::chr_page_marker(4));
    REQUIRE(restored.getNameTableMirroring() == NES::VERTICAL);
}
