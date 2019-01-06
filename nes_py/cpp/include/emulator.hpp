//  Program:      nes-py
//  File:         emulator.hpp
//  Description:  This class houses the logic and data for an NES emulator
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef EMULATOR_H
#define EMULATOR_H

#include "cartridge.hpp"
#include "controller.hpp"
#include "cpu.hpp"
#include "ppu.hpp"
#include "main_bus.hpp"
#include "mapper.hpp"
#include "picture_bus.hpp"
#include <stdint.h>
#include <string>

/// The width of the NES screen in pixels
const int NES_VIDEO_WIDTH = ScanlineVisibleDots;
/// The height of the NES screen in pixels
const int NES_VIDEO_HEIGHT = VisibleScanlines;
/// The number of cycles in approximately 1 frame
const int STEPS_PER_FRAME = 29781;

/// An NES Emulator and OpenAI Gym interface
class Emulator {

private:
    /// the path to the ROM for this environment
    std::string rom_path;
    /// the virtual cartridge with ROM and mapper data
    Cartridge cartridge;
    /// a pointer to the mapper on the cartridge
    Mapper* mapper;
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
    void DMA(uint8_t page);

public:
    /// Initialize a new emulator with a path to a ROM file.
    ///
    /// @param path the path to the ROM for the emulator to run
    ///
    Emulator(std::string path);

    /// Return a 32-bit pointer to the screen buffer's first address.
    ///
    /// @return a 32-bit pointer to the screen buffer's first address
    ///
    uint32_t* get_screen_buffer() { return ppu.get_screen_buffer(); };

    /// Return a 8-bit pointer to the RAM buffer's first address.
    ///
    /// @return a 8-bit pointer to the RAM buffer's first address
    ///
    uint8_t* get_memory_buffer() { return bus.get_memory_buffer(); };

    /// Load the ROM into the NES.
    void reset() { cpu.reset(bus); ppu.reset(); };

    /// Perform a discrete "step" of the NES by rendering 1 frame.
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
    void step(unsigned char action);

    /// Create a backup state on the emulator
    void backup();

    /// Restore the backup state on the emulator
    void restore();

};

#endif // EMULATOR_H
