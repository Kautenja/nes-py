//  Program:      nes-py
//  File:         mapper_test_helpers.hpp
//  Description:  Shared helpers for native mapper tests
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_EMU_TEST_SUPPORT_MAPPER_TEST_HELPERS_HPP
#define NES_EMU_TEST_SUPPORT_MAPPER_TEST_HELPERS_HPP

#include <memory>
#include <catch2/catch_test_macros.hpp>
#include "nes_emu/mapper_factory.hpp"

namespace NESTest {

inline std::unique_ptr<NES::Mapper> mapper_for(NES::Cartridge& cartridge) {
    std::unique_ptr<NES::Mapper> mapper = NES::MapperFactory(&cartridge);
    REQUIRE(mapper != nullptr);
    return mapper;
}

inline void write_mmc1_register(
    NES::Mapper& mapper,
    NES::NES_Address address,
    int value
) {
    for (int bit = 0; bit < 5; ++bit)
        mapper.writePRG(address, static_cast<NES::NES_Byte>((value >> bit) & 1));
}

inline NES::NES_Byte direct_prg_read(
    NES::Mapper& mapper,
    NES::NES_Address address
) {
    NES::NES_Address page_base = static_cast<NES::NES_Address>(
        0x8000 + (((address - 0x8000) / 0x2000) * 0x2000)
    );
    const NES::NES_Byte* page = mapper.getDirectPRGReadPage(page_base);
    REQUIRE(page != nullptr);
    return page[address & 0x1fff];
}

inline NES::NES_Byte direct_chr_read(
    NES::Mapper& mapper,
    NES::NES_Address address
) {
    NES::NES_Address page_base = static_cast<NES::NES_Address>(
        (address / 0x0400) * 0x0400
    );
    const NES::NES_Byte* page = mapper.getDirectCHRReadPage(page_base);
    REQUIRE(page != nullptr);
    return page[address & 0x03ff];
}

}  // namespace NESTest

#endif  // NES_EMU_TEST_SUPPORT_MAPPER_TEST_HELPERS_HPP
