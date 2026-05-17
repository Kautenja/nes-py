//  Program:      nes-py
//  File:         test_mapper_bank_helpers.cpp
//  Description:  Catch2 tests for mapper bank helper utilities
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include <vector>
#include <catch2/catch_test_macros.hpp>
#include "nes_emu/mapper_bank.hpp"
#include "nes_emu/test/nes_emu/support/test_mappers.hpp"

TEST_CASE("mapper bank windows resolve PRG and CHR windows safely", "[mapper]") {
    std::vector<NES::NES_Byte> prg_8k(4 * 0x2000, 0);
    NESTest::fill_bank_markers(prg_8k, 0x2000, 0x10);
    NES::MapperBank::BankWindow prg_8k_window;
    prg_8k_window.selectBank(prg_8k.size(), 0x2000, 2);
    REQUIRE(prg_8k_window.read(prg_8k, 0x8000, 0x8000) == 0x12);
    REQUIRE(prg_8k_window.read(prg_8k, 0x9fff, 0x8000) == 0x12);

    std::vector<NES::NES_Byte> prg_16k(4 * 0x4000, 0);
    NESTest::fill_bank_markers(prg_16k, 0x4000, 0x20);
    NES::MapperBank::BankWindow first_prg_16k;
    NES::MapperBank::BankWindow final_prg_16k;
    first_prg_16k.selectFirst(prg_16k.size(), 0x4000);
    final_prg_16k.selectFinal(prg_16k.size(), 0x4000);
    REQUIRE(first_prg_16k.read(prg_16k, 0x8000, 0x8000) == 0x20);
    REQUIRE(final_prg_16k.read(prg_16k, 0xc000, 0xc000) == 0x23);

    NES::MapperBank::BankWindow prg_32k_window;
    prg_32k_window.selectWindow(prg_16k.size(), 0x4000, 0x8000, 3);
    REQUIRE(prg_32k_window.bank() == 2);
    REQUIRE(prg_32k_window.read(prg_16k, 0x8000, 0x8000) == 0x22);
    REQUIRE(prg_32k_window.read(prg_16k, 0xc000, 0x8000) == 0x23);

    std::vector<NES::NES_Byte> chr_1k(16 * 0x0400, 0);
    NESTest::fill_bank_markers(chr_1k, 0x0400, 0x40);
    NES::MapperBank::BankWindow chr_1k_window;
    chr_1k_window.selectBank(chr_1k.size(), 0x0400, 7);
    REQUIRE(chr_1k_window.read(chr_1k, 0x0000, 0x0000) == 0x47);

    NES::MapperBank::BankWindow chr_2k_window;
    chr_2k_window.selectWindow(chr_1k.size(), 0x0400, 0x0800, 5);
    REQUIRE(chr_2k_window.bank() == 4);
    REQUIRE(chr_2k_window.read(chr_1k, 0x0000, 0x0000) == 0x44);
    REQUIRE(chr_2k_window.read(chr_1k, 0x0400, 0x0000) == 0x45);

    NES::MapperBank::BankWindow chr_4k_window;
    chr_4k_window.selectWindow(chr_1k.size(), 0x0400, 0x1000, 7);
    REQUIRE(chr_4k_window.bank() == 4);
    REQUIRE(chr_4k_window.read(chr_1k, 0x0000, 0x0000) == 0x44);
    REQUIRE(chr_4k_window.read(chr_1k, 0x0c00, 0x0000) == 0x47);

    NES::MapperBank::BankWindow chr_8k_window;
    chr_8k_window.selectWindow(chr_1k.size(), 0x0400, 0x2000, 13);
    REQUIRE(chr_8k_window.bank() == 8);
    REQUIRE(chr_8k_window.read(chr_1k, 0x0000, 0x0000) == 0x48);
    REQUIRE(chr_8k_window.read(chr_1k, 0x1c00, 0x0000) == 0x4f);
}

TEST_CASE("mapper bank helpers mask register values and bus conflicts", "[mapper]") {
    REQUIRE(NES::MapperBank::maskBankSelect(0x15, 4) == 1);
    REQUIRE(NES::MapperBank::maskBankSelect(3, 8, 2) == 2);
    REQUIRE(NES::MapperBank::resolveBusConflict(true, 0xf0, 0xcc) == 0xc0);
    REQUIRE(NES::MapperBank::resolveBusConflict(false, 0xf0, 0xcc) == 0xf0);
}
