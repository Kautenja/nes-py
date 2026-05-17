//  Program:      nes-py
//  File:         test_mapper_NROM.cpp
//  Description:  Catch2 coverage for native mapper 000 / NROM
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include <memory>
#include <catch2/catch_test_macros.hpp>
#include "nes_emu/test/nes_emu/support/emulator_inspector.hpp"
#include "nes_emu/test/nes_emu/support/mapper_test_helpers.hpp"
#include "nes_emu/test/nes_emu/support/synthetic_rom.hpp"

TEST_CASE("mapper 000 NROM maps fixed PRG and CHR data", "[mapper][nrom]") {
    NESTest::TemporaryROM rom("nrom_32k", 0, 2, 1, "vertical");
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(0));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(1));
    REQUIRE(mapper->readCHR(0x0100) == NESTest::chr_bank_marker(0));
    REQUIRE(mapper->getNameTableMirroring() == NES::VERTICAL);
}

TEST_CASE("mapper 000 mirrors 16 KiB PRG and snapshots PRG RAM", "[mapper][nrom]") {
    NESTest::TemporaryROM rom("nrom_16k", 0, 1, 1);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(0));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(0));

    mapper->writePRGRAM(0x6000, 0x2a);
    std::unique_ptr<NES::Mapper> backup = mapper->clone();
    mapper->writePRGRAM(0x6000, 0x7f);
    REQUIRE(mapper->readPRGRAM(0x6000) == 0x7f);
    REQUIRE(backup->readPRGRAM(0x6000) == 0x2a);
}

TEST_CASE("mapper 000 emulator save-state preserves PRG RAM", "[mapper][nrom]") {
    NESTest::TemporaryROM rom("nrom_emulator_backup", 0, 2, 1, "vertical");
    NES::Emulator emulator(rom.filename());
    emulator.reset();
    emulator.step();
    REQUIRE(emulator.get_screen_buffer() != nullptr);

    NES::Mapper& mapper = NES::EmulatorInspector::mapper(emulator);
    REQUIRE(mapper.getPRGRAMSize() == 8 * 1024);
    REQUIRE(mapper.getNameTableMirroring() == NES::VERTICAL);

    mapper.writePRGRAM(0x6000, 0x33);
    mapper.writePRGRAM(0x7fff, 0x44);
    emulator.backup();

    mapper.writePRGRAM(0x6000, 0xaa);
    mapper.writePRGRAM(0x7fff, 0xbb);
    REQUIRE(mapper.readPRGRAM(0x6000) == 0xaa);
    REQUIRE(mapper.readPRGRAM(0x7fff) == 0xbb);

    emulator.restore();
    NES::Mapper& restored = NES::EmulatorInspector::mapper(emulator);
    REQUIRE(restored.readPRGRAM(0x6000) == 0x33);
    REQUIRE(restored.readPRGRAM(0x7fff) == 0x44);
    REQUIRE(restored.getNameTableMirroring() == NES::VERTICAL);

    restored.writePRGRAM(0x6000, 0x55);
    REQUIRE(restored.readPRGRAM(0x6000) == 0x55);

    emulator.restore();
    NES::Mapper& restored_again = NES::EmulatorInspector::mapper(emulator);
    REQUIRE(restored_again.readPRGRAM(0x6000) == 0x33);
    REQUIRE(restored_again.readPRGRAM(0x7fff) == 0x44);
}
