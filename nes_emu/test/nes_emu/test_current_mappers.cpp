//  Program:      nes-py
//  File:         test_current_mappers.cpp
//  Description:  Catch2 coverage for currently supported native mappers
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include <memory>
#include <catch2/catch_test_macros.hpp>
#include "nes_emu/mapper_factory.hpp"
#include "nes_emu/test/nes_emu/support/synthetic_rom.hpp"

namespace {

std::unique_ptr<NES::Mapper> mapper_for(NES::Cartridge& cartridge) {
    std::unique_ptr<NES::Mapper> mapper = NES::MapperFactory(&cartridge);
    REQUIRE(mapper != nullptr);
    return mapper;
}

void write_mmc1_register(NES::Mapper& mapper, NES::NES_Address address, int value) {
    for (int bit = 0; bit < 5; ++bit)
        mapper.writePRG(address, static_cast<NES::NES_Byte>((value >> bit) & 1));
}

}  // namespace

TEST_CASE("mapper 000 NROM maps fixed PRG and CHR data", "[mapper]") {
    NESTest::TemporaryROM rom("nrom_32k", 0, 2, 1, "vertical");
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = mapper_for(cartridge);

    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(0));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(1));
    REQUIRE(mapper->readCHR(0x0100) == NESTest::chr_bank_marker(0));
    REQUIRE(mapper->getNameTableMirroring() == NES::VERTICAL);
}

TEST_CASE("mapper 000 mirrors 16 KiB PRG and snapshots PRG RAM", "[mapper]") {
    NESTest::TemporaryROM rom("nrom_16k", 0, 1, 1);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = mapper_for(cartridge);

    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(0));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(0));

    mapper->writePRGRAM(0x6000, 0x2a);
    std::unique_ptr<NES::Mapper> backup = mapper->clone();
    mapper->writePRGRAM(0x6000, 0x7f);
    REQUIRE(mapper->readPRGRAM(0x6000) == 0x7f);
    REQUIRE(backup->readPRGRAM(0x6000) == 0x2a);
}

TEST_CASE("mapper 001 SxROM serial writes select PRG banks and mirroring", "[mapper]") {
    NESTest::TemporaryROM rom("sxrom_prg", 1, 4, 0);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = mapper_for(cartridge);

    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(0));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(3));

    write_mmc1_register(*mapper, 0x8000, 0x0e);
    REQUIRE(mapper->getNameTableMirroring() == NES::VERTICAL);
    write_mmc1_register(*mapper, 0xe000, 0x02);
    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(2));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(3));

    write_mmc1_register(*mapper, 0x8000, 0x0f);
    REQUIRE(mapper->getNameTableMirroring() == NES::HORIZONTAL);
}

TEST_CASE("mapper 001 reset bit discards partial serial writes", "[mapper]") {
    NESTest::TemporaryROM rom("sxrom_reset", 1, 4, 0);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = mapper_for(cartridge);

    write_mmc1_register(*mapper, 0x8000, 0x08);
    write_mmc1_register(*mapper, 0xe000, 0x02);
    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(0));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(2));

    mapper->writePRG(0xe000, 0x01);
    mapper->writePRG(0xe000, 0x01);
    mapper->writePRG(0x8000, 0x80);
    write_mmc1_register(*mapper, 0xe000, 0x01);

    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(1));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(3));
}

TEST_CASE("mapper 001 supports all MMC1 mirroring modes", "[mapper]") {
    NESTest::TemporaryROM rom("sxrom_mirroring", 1, 4, 0);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = mapper_for(cartridge);

    write_mmc1_register(*mapper, 0x8000, 0x0c);
    REQUIRE(mapper->getNameTableMirroring() == NES::ONE_SCREEN_LOWER);
    write_mmc1_register(*mapper, 0x8000, 0x0d);
    REQUIRE(mapper->getNameTableMirroring() == NES::ONE_SCREEN_HIGHER);
    write_mmc1_register(*mapper, 0x8000, 0x0e);
    REQUIRE(mapper->getNameTableMirroring() == NES::VERTICAL);
    write_mmc1_register(*mapper, 0x8000, 0x0f);
    REQUIRE(mapper->getNameTableMirroring() == NES::HORIZONTAL);
}

TEST_CASE("mapper 001 masks PRG bank modes safely", "[mapper]") {
    NESTest::TemporaryROM rom("sxrom_prg_modes", 1, 4, 0);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = mapper_for(cartridge);

    write_mmc1_register(*mapper, 0x8000, 0x00);
    write_mmc1_register(*mapper, 0xe000, 0x03);
    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(2));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(3));

    write_mmc1_register(*mapper, 0x8000, 0x08);
    write_mmc1_register(*mapper, 0xe000, 0x06);
    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(0));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(2));

    write_mmc1_register(*mapper, 0x8000, 0x0c);
    write_mmc1_register(*mapper, 0xe000, 0x06);
    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(2));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(3));
}

TEST_CASE("mapper 001 supports 4 KiB and 8 KiB CHR banking", "[mapper]") {
    NESTest::TemporaryROM rom("sxrom_chr", 1, 4, 4, "horizontal", true);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = mapper_for(cartridge);

    write_mmc1_register(*mapper, 0xa000, 0x03);
    REQUIRE(mapper->readCHR(0x0000) == NESTest::chr_page_marker(2));
    REQUIRE(mapper->readCHR(0x1000) == NESTest::chr_page_marker(3));

    write_mmc1_register(*mapper, 0x8000, 0x1e);
    write_mmc1_register(*mapper, 0xa000, 0x05);
    write_mmc1_register(*mapper, 0xc000, 0x06);
    REQUIRE(mapper->readCHR(0x0000) == NESTest::chr_page_marker(5));
    REQUIRE(mapper->readCHR(0x1000) == NESTest::chr_page_marker(6));
}

TEST_CASE("mapper 001 protects PRG RAM and clones CHR RAM state", "[mapper]") {
    NESTest::TemporaryROM rom("sxrom_backup", 1, 4, 0);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = mapper_for(cartridge);

    mapper->writePRGRAM(0x6000, 0x11);
    write_mmc1_register(*mapper, 0xe000, 0x10);
    mapper->writePRGRAM(0x6000, 0x22);
    REQUIRE(mapper->readPRGRAM(0x6000) == 0x11);

    write_mmc1_register(*mapper, 0xe000, 0x00);
    mapper->writePRGRAM(0x6000, 0x33);
    REQUIRE(mapper->readPRGRAM(0x6000) == 0x33);

    write_mmc1_register(*mapper, 0x8000, 0x0f);
    write_mmc1_register(*mapper, 0xe000, 0x01);
    mapper->writeCHR(0x0123, 0x5a);
    std::unique_ptr<NES::Mapper> backup = mapper->clone();

    write_mmc1_register(*mapper, 0x8000, 0x0e);
    write_mmc1_register(*mapper, 0xe000, 0x02);
    mapper->writeCHR(0x0123, 0xa5);
    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(2));
    REQUIRE(mapper->readCHR(0x0123) == 0xa5);
    REQUIRE(mapper->getNameTableMirroring() == NES::VERTICAL);

    REQUIRE(backup->readPRG(0x9000) == NESTest::prg_bank_marker(1));
    REQUIRE(backup->readPRG(0xd000) == NESTest::prg_bank_marker(3));
    REQUIRE(backup->readCHR(0x0123) == 0x5a);
    REQUIRE(backup->getNameTableMirroring() == NES::HORIZONTAL);
}

TEST_CASE("mapper 002 UxROM switches PRG and provides CHR RAM", "[mapper]") {
    NESTest::TemporaryROM rom("uxrom", 2, 4, 0);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = mapper_for(cartridge);

    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(0));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(3));

    mapper->writePRG(0x8000, 0x02);
    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(2));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(3));

    mapper->writeCHR(0x0456, 0x3c);
    REQUIRE(mapper->readCHR(0x0456) == 0x3c);

    mapper->writePRG(0x8000, 0x05);
    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(1));
}

TEST_CASE("mapper 003 CNROM switches CHR banks and masks selects", "[mapper]") {
    NESTest::TemporaryROM rom("cnrom", 3, 2, 4);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = mapper_for(cartridge);

    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(0));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(1));
    REQUIRE(mapper->readCHR(0x0100) == NESTest::chr_bank_marker(0));

    mapper->writePRG(0x8000, 0x02);
    REQUIRE(mapper->readCHR(0x0100) == NESTest::chr_bank_marker(2));
    mapper->writePRG(0x8000, 0x03);
    REQUIRE(mapper->readCHR(0x0100) == NESTest::chr_bank_marker(3));
}

TEST_CASE("mapper 003 masks CHR bank selects to available banks", "[mapper]") {
    NESTest::TemporaryROM rom("cnrom_masked", 3, 2, 2);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = mapper_for(cartridge);

    mapper->writePRG(0x8000, 0x03);
    REQUIRE(mapper->readCHR(0x0100) == NESTest::chr_bank_marker(1));
}
