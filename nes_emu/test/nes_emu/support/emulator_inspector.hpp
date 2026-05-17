//  Program:      nes-py
//  File:         emulator_inspector.hpp
//  Description:  Test-only accessors for native emulator internals
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_EMU_TEST_SUPPORT_EMULATOR_INSPECTOR_HPP
#define NES_EMU_TEST_SUPPORT_EMULATOR_INSPECTOR_HPP

#include "nes_emu/emulator.hpp"

namespace NES {

struct EmulatorInspector {
    static Mapper& mapper(Emulator& emulator) {
        return *emulator.mapper;
    }
};

}  // namespace NES

#endif  // NES_EMU_TEST_SUPPORT_EMULATOR_INSPECTOR_HPP
