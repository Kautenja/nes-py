//  Program:      nes-py
//  File:         test_mapper_SxROM.cpp
//  Description:  Catch2 coverage for native mapper 001 / SxROM
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include <memory>
#include <catch2/catch_test_macros.hpp>
#include "nes_emu/test/nes_emu/support/emulator_inspector.hpp"
#include "nes_emu/test/nes_emu/support/mapper_test_helpers.hpp"
#include "nes_emu/test/nes_emu/support/synthetic_rom.hpp"

TEST_CASE(
    "mapper 001 SxROM serial writes select PRG banks and mirroring",
    "[mapper][sxrom]"
) {
    NESTest::TemporaryROM rom("sxrom_prg", 1, 4, 0);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(0));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(3));

    NESTest::write_mmc1_register(*mapper, 0x8000, 0x0e);
    REQUIRE(mapper->getNameTableMirroring() == NES::VERTICAL);
    NESTest::write_mmc1_register(*mapper, 0xe000, 0x02);
    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(2));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(3));

    NESTest::write_mmc1_register(*mapper, 0x8000, 0x0f);
    REQUIRE(mapper->getNameTableMirroring() == NES::HORIZONTAL);
}

TEST_CASE("mapper 001 reset bit discards partial serial writes", "[mapper][sxrom]") {
    NESTest::TemporaryROM rom("sxrom_reset", 1, 4, 0);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    NESTest::write_mmc1_register(*mapper, 0x8000, 0x08);
    NESTest::write_mmc1_register(*mapper, 0xe000, 0x02);
    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(0));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(2));

    mapper->writePRG(0xe000, 0x01);
    mapper->writePRG(0xe000, 0x01);
    mapper->writePRG(0x8000, 0x80);
    NESTest::write_mmc1_register(*mapper, 0xe000, 0x01);

    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(1));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(3));
}

TEST_CASE("mapper 001 supports all MMC1 mirroring modes", "[mapper][sxrom]") {
    NESTest::TemporaryROM rom("sxrom_mirroring", 1, 4, 0);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    NESTest::write_mmc1_register(*mapper, 0x8000, 0x0c);
    REQUIRE(mapper->getNameTableMirroring() == NES::ONE_SCREEN_LOWER);
    NESTest::write_mmc1_register(*mapper, 0x8000, 0x0d);
    REQUIRE(mapper->getNameTableMirroring() == NES::ONE_SCREEN_HIGHER);
    NESTest::write_mmc1_register(*mapper, 0x8000, 0x0e);
    REQUIRE(mapper->getNameTableMirroring() == NES::VERTICAL);
    NESTest::write_mmc1_register(*mapper, 0x8000, 0x0f);
    REQUIRE(mapper->getNameTableMirroring() == NES::HORIZONTAL);
}

TEST_CASE("mapper 001 masks PRG bank modes safely", "[mapper][sxrom]") {
    NESTest::TemporaryROM rom("sxrom_prg_modes", 1, 4, 0);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    NESTest::write_mmc1_register(*mapper, 0x8000, 0x00);
    NESTest::write_mmc1_register(*mapper, 0xe000, 0x03);
    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(2));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(3));

    NESTest::write_mmc1_register(*mapper, 0x8000, 0x08);
    NESTest::write_mmc1_register(*mapper, 0xe000, 0x06);
    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(0));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(2));

    NESTest::write_mmc1_register(*mapper, 0x8000, 0x0c);
    NESTest::write_mmc1_register(*mapper, 0xe000, 0x06);
    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(2));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(3));
}

TEST_CASE("mapper 001 supports 4 KiB and 8 KiB CHR banking", "[mapper][sxrom]") {
    NESTest::TemporaryROM rom("sxrom_chr", 1, 4, 4, "horizontal", true);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    NESTest::write_mmc1_register(*mapper, 0xa000, 0x03);
    REQUIRE(mapper->readCHR(0x0000) == NESTest::chr_page_marker(2));
    REQUIRE(mapper->readCHR(0x1000) == NESTest::chr_page_marker(3));

    NESTest::write_mmc1_register(*mapper, 0x8000, 0x1e);
    NESTest::write_mmc1_register(*mapper, 0xa000, 0x05);
    NESTest::write_mmc1_register(*mapper, 0xc000, 0x06);
    REQUIRE(mapper->readCHR(0x0000) == NESTest::chr_page_marker(5));
    REQUIRE(mapper->readCHR(0x1000) == NESTest::chr_page_marker(6));
}

TEST_CASE("mapper 001 protects PRG RAM and clones CHR RAM state", "[mapper][sxrom]") {
    NESTest::TemporaryROM rom("sxrom_backup", 1, 4, 0);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    mapper->writePRGRAM(0x6000, 0x11);
    NESTest::write_mmc1_register(*mapper, 0xe000, 0x10);
    mapper->writePRGRAM(0x6000, 0x22);
    REQUIRE(mapper->readPRGRAM(0x6000) == 0x11);

    NESTest::write_mmc1_register(*mapper, 0xe000, 0x00);
    mapper->writePRGRAM(0x6000, 0x33);
    REQUIRE(mapper->readPRGRAM(0x6000) == 0x33);

    NESTest::write_mmc1_register(*mapper, 0x8000, 0x0f);
    NESTest::write_mmc1_register(*mapper, 0xe000, 0x01);
    mapper->writeCHR(0x0123, 0x5a);
    std::unique_ptr<NES::Mapper> backup = mapper->clone();

    NESTest::write_mmc1_register(*mapper, 0x8000, 0x0e);
    NESTest::write_mmc1_register(*mapper, 0xe000, 0x02);
    mapper->writeCHR(0x0123, 0xa5);
    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(2));
    REQUIRE(mapper->readCHR(0x0123) == 0xa5);
    REQUIRE(mapper->getNameTableMirroring() == NES::VERTICAL);

    REQUIRE(backup->readPRG(0x9000) == NESTest::prg_bank_marker(1));
    REQUIRE(backup->readPRG(0xd000) == NESTest::prg_bank_marker(3));
    REQUIRE(backup->readCHR(0x0123) == 0x5a);
    REQUIRE(backup->getNameTableMirroring() == NES::HORIZONTAL);
}

TEST_CASE("mapper 001 emulator save-state preserves MMC1 state", "[mapper][sxrom]") {
    NESTest::TemporaryROM rom("sxrom_emulator_backup", 1, 4, 0);
    NES::Emulator emulator(rom.filename());
    NES::Mapper& mapper = NES::EmulatorInspector::mapper(emulator);

    mapper.writePRGRAM(0x6000, 0x33);
    NESTest::write_mmc1_register(mapper, 0x8000, 0x0f);
    NESTest::write_mmc1_register(mapper, 0xe000, 0x11);
    mapper.writePRGRAM(0x6000, 0x44);
    mapper.writeCHR(0x0123, 0x5a);
    REQUIRE(mapper.readPRGRAM(0x6000) == 0x33);

    emulator.backup();

    NESTest::write_mmc1_register(mapper, 0x8000, 0x0e);
    NESTest::write_mmc1_register(mapper, 0xe000, 0x02);
    mapper.writePRGRAM(0x6000, 0x99);
    mapper.writeCHR(0x0123, 0xa5);
    REQUIRE(mapper.readPRG(0x9000) == NESTest::prg_bank_marker(2));
    REQUIRE(mapper.readCHR(0x0123) == 0xa5);
    REQUIRE(mapper.readPRGRAM(0x6000) == 0x99);
    REQUIRE(mapper.getNameTableMirroring() == NES::VERTICAL);

    emulator.restore();
    NES::Mapper& restored = NES::EmulatorInspector::mapper(emulator);
    REQUIRE(restored.readPRG(0x9000) == NESTest::prg_bank_marker(1));
    REQUIRE(restored.readPRG(0xd000) == NESTest::prg_bank_marker(3));
    REQUIRE(restored.readCHR(0x0123) == 0x5a);
    REQUIRE(restored.readPRGRAM(0x6000) == 0x33);
    REQUIRE(restored.getNameTableMirroring() == NES::HORIZONTAL);

    restored.writePRGRAM(0x6000, 0x77);
    REQUIRE(restored.readPRGRAM(0x6000) == 0x33);
}
