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

    static bool uses_instruction_batching(Emulator& emulator) {
        return emulator.can_batch_cpu_instructions();
    }

    static void step_cycle_by_cycle(Emulator& emulator) {
        emulator.step_cycle_by_cycle();
    }

    static void step_instruction_batched(Emulator& emulator) {
        emulator.step_instruction_batched();
    }

    static CPU::Snapshot cpu_snapshot(const Emulator& emulator) {
        return emulator.cpu.save_state();
    }

    static MainBus::State bus_state(const Emulator& emulator) {
        return emulator.bus.save_state();
    }

    static PPU::Snapshot ppu_snapshot(const Emulator& emulator) {
        return emulator.ppu.save_state();
    }
};

}  // namespace NES

#endif  // NES_EMU_TEST_SUPPORT_EMULATOR_INSPECTOR_HPP
