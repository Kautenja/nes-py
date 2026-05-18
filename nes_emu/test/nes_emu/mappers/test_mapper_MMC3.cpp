//  Program:      nes-py
//  File:         test_mapper_MMC3.cpp
//  Description:  Catch2 coverage for native mapper 004 / MMC3
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include <memory>
#include <catch2/catch_test_macros.hpp>
#include "nes_emu/mapper_factory.hpp"
#include "nes_emu/test/nes_emu/support/mapper_test_helpers.hpp"
#include "nes_emu/test/nes_emu/support/synthetic_rom.hpp"

namespace {

const int MMC3_A12_LOW_FILTER_OBSERVATIONS = 8;

void write_mmc3_bank(
    NES::Mapper& mapper,
    NES::NES_Byte bank_register,
    NES::NES_Byte value,
    NES::NES_Byte mode_bits = 0
) {
    mapper.writePRG(0x8000, mode_bits | (bank_register & 0x07));
    mapper.writePRG(0x8001, value);
}

void observe_low_a12(NES::Mapper& mapper, int observations) {
    for (int index = 0; index < observations; ++index)
        mapper.onPPUAddress(0x0000);
}

void filtered_a12_clock(NES::Mapper& mapper) {
    observe_low_a12(mapper, MMC3_A12_LOW_FILTER_OBSERVATIONS);
    mapper.onPPUAddress(0x1000);
}

}  // namespace

TEST_CASE("mapper 004 MMC3 registers in the native factory", "[mapper][mmc3]") {
    NESTest::TemporaryROM rom("mmc3_registry", 4, 2, 1);
    NES::Cartridge cartridge = rom.load();

    REQUIRE(NES::IsMapperSupported(4));
    REQUIRE(NESTest::mapper_for(cartridge) != nullptr);
}

TEST_CASE("mapper 004 MMC3 switches 8 KiB PRG banks in both modes", "[mapper][mmc3]") {
    NESTest::TemporaryROM rom(
        "mmc3_prg",
        4,
        4,
        1,
        "horizontal",
        false,
        true
    );
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_page_marker(0));
    REQUIRE(mapper->readPRG(0xb000) == NESTest::prg_page_marker(1));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_page_marker(6));
    REQUIRE(mapper->readPRG(0xf000) == NESTest::prg_page_marker(7));

    write_mmc3_bank(*mapper, 6, 3);
    write_mmc3_bank(*mapper, 7, 4);
    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_page_marker(3));
    REQUIRE(mapper->readPRG(0xb000) == NESTest::prg_page_marker(4));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_page_marker(6));
    REQUIRE(mapper->readPRG(0xf000) == NESTest::prg_page_marker(7));

    mapper->writePRG(0x8000, 0x40);
    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_page_marker(6));
    REQUIRE(mapper->readPRG(0xb000) == NESTest::prg_page_marker(4));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_page_marker(3));
    REQUIRE(mapper->readPRG(0xf000) == NESTest::prg_page_marker(7));
}

TEST_CASE("mapper 004 MMC3 switches 2 KiB and 1 KiB CHR banks", "[mapper][mmc3]") {
    NESTest::TemporaryROM rom(
        "mmc3_chr",
        4,
        2,
        2,
        "horizontal",
        false,
        false,
        true
    );
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    write_mmc3_bank(*mapper, 0, 7);
    write_mmc3_bank(*mapper, 1, 8);
    write_mmc3_bank(*mapper, 2, 10);
    write_mmc3_bank(*mapper, 3, 11);
    write_mmc3_bank(*mapper, 4, 12);
    write_mmc3_bank(*mapper, 5, 13);

    REQUIRE(mapper->readCHR(0x0000) == NESTest::chr_kib_marker(6));
    REQUIRE(mapper->readCHR(0x0400) == NESTest::chr_kib_marker(7));
    REQUIRE(mapper->readCHR(0x0800) == NESTest::chr_kib_marker(8));
    REQUIRE(mapper->readCHR(0x0c00) == NESTest::chr_kib_marker(9));
    REQUIRE(mapper->readCHR(0x1000) == NESTest::chr_kib_marker(10));
    REQUIRE(mapper->readCHR(0x1400) == NESTest::chr_kib_marker(11));
    REQUIRE(mapper->readCHR(0x1800) == NESTest::chr_kib_marker(12));
    REQUIRE(mapper->readCHR(0x1c00) == NESTest::chr_kib_marker(13));

    mapper->writePRG(0x8000, 0x80);
    REQUIRE(mapper->readCHR(0x0000) == NESTest::chr_kib_marker(10));
    REQUIRE(mapper->readCHR(0x0400) == NESTest::chr_kib_marker(11));
    REQUIRE(mapper->readCHR(0x0800) == NESTest::chr_kib_marker(12));
    REQUIRE(mapper->readCHR(0x0c00) == NESTest::chr_kib_marker(13));
    REQUIRE(mapper->readCHR(0x1000) == NESTest::chr_kib_marker(6));
    REQUIRE(mapper->readCHR(0x1400) == NESTest::chr_kib_marker(7));
    REQUIRE(mapper->readCHR(0x1800) == NESTest::chr_kib_marker(8));
    REQUIRE(mapper->readCHR(0x1c00) == NESTest::chr_kib_marker(9));
}

TEST_CASE("mapper 004 MMC3 direct CHR pages stay valid with A12 observation", "[mapper][mmc3]") {
    NESTest::TemporaryROM rom(
        "mmc3_direct_chr_pages",
        4,
        2,
        2,
        "horizontal",
        false,
        false,
        true
    );
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    REQUIRE(mapper->observesPPUAddresses());
    REQUIRE(mapper->allowsDirectCHRReadWithPPUAddressObservations());
    REQUIRE(mapper->allowsBackgroundTileCacheWithPPUAddressObservations());
    REQUIRE(NESTest::direct_chr_read(*mapper, 0x0000) == NESTest::chr_kib_marker(0));
    REQUIRE(NESTest::direct_chr_read(*mapper, 0x0400) == NESTest::chr_kib_marker(1));

    write_mmc3_bank(*mapper, 0, 7);
    write_mmc3_bank(*mapper, 1, 8);
    write_mmc3_bank(*mapper, 2, 10);
    write_mmc3_bank(*mapper, 3, 11);
    write_mmc3_bank(*mapper, 4, 12);
    write_mmc3_bank(*mapper, 5, 13);

    REQUIRE(NESTest::direct_chr_read(*mapper, 0x0000) == NESTest::chr_kib_marker(6));
    REQUIRE(NESTest::direct_chr_read(*mapper, 0x0400) == NESTest::chr_kib_marker(7));
    REQUIRE(NESTest::direct_chr_read(*mapper, 0x0800) == NESTest::chr_kib_marker(8));
    REQUIRE(NESTest::direct_chr_read(*mapper, 0x0c00) == NESTest::chr_kib_marker(9));
    REQUIRE(NESTest::direct_chr_read(*mapper, 0x1000) == NESTest::chr_kib_marker(10));
    REQUIRE(NESTest::direct_chr_read(*mapper, 0x1400) == NESTest::chr_kib_marker(11));
    REQUIRE(NESTest::direct_chr_read(*mapper, 0x1800) == NESTest::chr_kib_marker(12));
    REQUIRE(NESTest::direct_chr_read(*mapper, 0x1c00) == NESTest::chr_kib_marker(13));
}

TEST_CASE("mapper 004 MMC3 controls mirroring except four-screen carts", "[mapper][mmc3]") {
    NESTest::TemporaryROM rom("mmc3_mirroring", 4, 2, 1);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    REQUIRE(mapper->getNameTableMirroring() == NES::HORIZONTAL);
    mapper->writePRG(0xa000, 0x00);
    REQUIRE(mapper->getNameTableMirroring() == NES::VERTICAL);
    mapper->writePRG(0xa000, 0x01);
    REQUIRE(mapper->getNameTableMirroring() == NES::HORIZONTAL);

    NESTest::TemporaryROM four_screen_rom(
        "mmc3_four_screen",
        4,
        2,
        1,
        "four-screen"
    );
    NES::Cartridge four_screen_cartridge = four_screen_rom.load();
    std::unique_ptr<NES::Mapper> four_screen_mapper =
        NESTest::mapper_for(four_screen_cartridge);

    REQUIRE(four_screen_mapper->getNameTableMirroring() == NES::FOUR_SCREEN);
    four_screen_mapper->writePRG(0xa000, 0x00);
    REQUIRE(four_screen_mapper->getNameTableMirroring() == NES::FOUR_SCREEN);
}

TEST_CASE("mapper 004 MMC3 applies PRG RAM enable and protect bits", "[mapper][mmc3]") {
    NESTest::TemporaryROM rom("mmc3_prg_ram", 4, 2, 1);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    mapper->writePRGRAM(0x6000, 0x11);
    REQUIRE(mapper->readPRGRAM(0x6000) == 0x11);
    REQUIRE(mapper->getPRGRAMPointer(0x6000) != nullptr);

    mapper->writePRG(0xa001, 0x00);
    mapper->writePRGRAM(0x6000, 0x22);
    REQUIRE(mapper->readPRGRAM(0x6000) == 0x00);
    REQUIRE(mapper->getPRGRAMPointer(0x6000) == nullptr);

    mapper->writePRG(0xa001, 0xc0);
    mapper->writePRGRAM(0x6000, 0x33);
    REQUIRE(mapper->readPRGRAM(0x6000) == 0x11);

    mapper->writePRG(0xa001, 0x80);
    mapper->writePRGRAM(0x6000, 0x44);
    REQUIRE(mapper->readPRGRAM(0x6000) == 0x44);
}

TEST_CASE("mapper 004 MMC3 clocks IRQs from filtered PPU A12 edges", "[mapper][mmc3]") {
    NESTest::TemporaryROM rom("mmc3_irq", 4, 2, 1);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);
    int irq_count = 0;
    mapper->setIRQCallback([&]() { ++irq_count; });

    REQUIRE(mapper->observesPPUAddresses());
    REQUIRE_FALSE(mapper->observesCPUCycles());

    mapper->writePRG(0xc000, 0x02);
    mapper->writePRG(0xc001, 0x00);
    mapper->writePRG(0xe001, 0x00);

    observe_low_a12(*mapper, MMC3_A12_LOW_FILTER_OBSERVATIONS - 1);
    mapper->onPPUAddress(0x1000);
    REQUIRE(irq_count == 0);

    filtered_a12_clock(*mapper);
    filtered_a12_clock(*mapper);
    filtered_a12_clock(*mapper);
    REQUIRE(irq_count == 1);

    mapper->writePRG(0xe000, 0x00);
    filtered_a12_clock(*mapper);
    filtered_a12_clock(*mapper);
    REQUIRE(irq_count == 1);

    mapper->writePRG(0xc000, 0x00);
    mapper->writePRG(0xc001, 0x00);
    mapper->writePRG(0xe001, 0x00);
    filtered_a12_clock(*mapper);
    REQUIRE(irq_count == 2);
}

TEST_CASE("mapper 004 MMC3 only invalidates direct pages for bank-affecting writes", "[mapper][mmc3]") {
    NESTest::TemporaryROM rom("mmc3_direct_page_invalidation", 4, 2, 1);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    REQUIRE(mapper->invalidatesDirectPRGReadPagesOnWrite(0x8000, 0x00));
    REQUIRE(mapper->invalidatesDirectCHRReadPagesOnWrite(0x8000, 0x00));

    mapper->writePRG(0x8000, 0x02);
    REQUIRE_FALSE(mapper->invalidatesDirectPRGReadPagesOnWrite(0x8001, 0x00));
    REQUIRE(mapper->invalidatesDirectCHRReadPagesOnWrite(0x8001, 0x00));

    mapper->writePRG(0x8000, 0x06);
    REQUIRE(mapper->invalidatesDirectPRGReadPagesOnWrite(0x8001, 0x00));
    REQUIRE_FALSE(mapper->invalidatesDirectCHRReadPagesOnWrite(0x8001, 0x00));

    REQUIRE_FALSE(mapper->invalidatesDirectPRGReadPagesOnWrite(0xc000, 0x00));
    REQUIRE_FALSE(mapper->invalidatesDirectCHRReadPagesOnWrite(0xc000, 0x00));
    REQUIRE_FALSE(mapper->invalidatesDirectPRGReadPagesOnWrite(0xe001, 0x00));
    REQUIRE_FALSE(mapper->invalidatesDirectCHRReadPagesOnWrite(0xe001, 0x00));
}

TEST_CASE("mapper 004 MMC3 clone preserves mapper state", "[mapper][mmc3]") {
    NESTest::TemporaryROM rom(
        "mmc3_clone",
        4,
        4,
        2,
        "horizontal",
        false,
        true,
        true
    );
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    write_mmc3_bank(*mapper, 6, 3);
    write_mmc3_bank(*mapper, 0, 7);
    mapper->writePRG(0xa000, 0x00);
    mapper->writePRGRAM(0x6000, 0x44);
    mapper->writePRG(0xa001, 0xc0);
    mapper->writePRG(0xc000, 0x01);
    mapper->writePRG(0xc001, 0x00);
    mapper->writePRG(0xe001, 0x00);
    observe_low_a12(*mapper, MMC3_A12_LOW_FILTER_OBSERVATIONS - 1);

    std::unique_ptr<NES::Mapper> backup = mapper->clone();

    write_mmc3_bank(*mapper, 6, 5);
    write_mmc3_bank(*mapper, 0, 9);
    mapper->writePRG(0xa000, 0x01);
    mapper->writePRG(0xa001, 0x80);
    mapper->writePRGRAM(0x6000, 0x99);

    REQUIRE(backup->readPRG(0x9000) == NESTest::prg_page_marker(3));
    REQUIRE(backup->readCHR(0x0000) == NESTest::chr_kib_marker(6));
    REQUIRE(backup->getNameTableMirroring() == NES::VERTICAL);
    REQUIRE(backup->readPRGRAM(0x6000) == 0x44);
    backup->writePRGRAM(0x6000, 0x77);
    REQUIRE(backup->readPRGRAM(0x6000) == 0x44);

    int backup_irq_count = 0;
    backup->setIRQCallback([&]() { ++backup_irq_count; });
    backup->onPPUAddress(0x0000);
    backup->onPPUAddress(0x1000);
    REQUIRE(backup_irq_count == 0);
    filtered_a12_clock(*backup);
    REQUIRE(backup_irq_count == 1);
}
