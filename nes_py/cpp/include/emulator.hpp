//  Program:      nes-py
//  File:         emulator.hpp
//  Description:  This class houses the logic and data for an NES emulator
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef EMULATOR_HPP
#define EMULATOR_HPP

#include "common.hpp"
#include "cartridge.hpp"
#include "controller.hpp"
#include "cpu.hpp"
#include "ppu.hpp"
#include "main_bus.hpp"
#include "mapper.hpp"
#include "picture_bus.hpp"
#include <string>

/// An NES Emulator and OpenAI Gym interface
class Emulator {

private:
    /// The number of cycles in 1 frame
    const static int CYCLES_PER_FRAME = 29781;

    /// the virtual cartridge with ROM and mapper data
    Cartridge cartridge;
    /// the 2 controllers on the emulator
    Controller controller1, controller2;

    /// the main data bus of the emulator
    MainBus bus;
    /// the picture bus from the PPU of the emulator
    PictureBus picture_bus;
    /// The emulator's CPU
    CPU cpu;
    /// the emulators' PPU
    PPU ppu;

    /// the main data bus of the emulator
    MainBus backup_bus;
    /// the picture bus from the PPU of the emulator
    PictureBus backup_picture_bus;
    /// The emulator's CPU
    CPU backup_cpu;
    /// the emulators' PPU
    PPU backup_ppu;

    /// Skip DMA cycle and perform a DMA copy.
    void DMA(NES_Byte page);

public:
    /// The width of the NES screen in pixels
    const static int WIDTH = SCANLINE_VISIBLE_DOTS;
    /// The height of the NES screen in pixels
    const static int HEIGHT = VISIBLE_SCANLINES;

    /// Initialize a new emulator with a path to a ROM file.
    ///
    /// @param rom_path the path to the ROM for the emulator to run
    ///
    Emulator(std::string rom_path);

    /// Return a 32-bit pointer to the screen buffer's first address.
    ///
    /// @return a 32-bit pointer to the screen buffer's first address
    ///
    inline NES_Pixel* get_screen_buffer() { return ppu.get_screen_buffer(); };

    /// Return a 8-bit pointer to the RAM buffer's first address.
    ///
    /// @return a 8-bit pointer to the RAM buffer's first address
    ///
    inline NES_Byte* get_memory_buffer() { return bus.get_memory_buffer(); };

    /// Load the ROM into the NES.
    inline void reset() { cpu.reset(bus); ppu.reset(); };

    /// Perform a step on the emulator, i.e., a single frame.
    ///
    /// @param action the controller bitmap of which buttons to press.
    ///
    /// The bitmap uses 1 for "pressed" and 0 for "not pressed".
    /// It uses the following mapping of bits to buttons:
    /// 7: RIGHT
    /// 6: LEFT
    /// 5: DOWN
    /// 4: UP
    /// 3: START
    /// 2: SELECT
    /// 1: B
    /// 0: A
    ///
    void step(NES_Byte action);

    /// Create a backup state on the emulator
    void backup();

    /// Restore the backup state on the emulator
    void restore();

};

#endif // EMULATOR_HPP
