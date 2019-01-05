#ifndef EMULATOR_H
#define EMULATOR_H
#include <string>
#include <stdint.h>
#include "cartridge.hpp"
#include "controller.hpp"
#include "cpu.hpp"
#include "ppu.hpp"
#include "main_bus.hpp"
#include "mapper.hpp"
#include "picture_bus.hpp"

/// The width of the NES screen in pixels
const int NESVideoWidth = ScanlineVisibleDots;
/// The height of the NES screen in pixels
const int NESVideoHeight = VisibleScanlines;

/// An NES Emulator and OpenAI Gym interface
class Emulator {

private:
    /// the path to the ROM for this environment
    std::string rom_path;

    /// the main data bus of the emulator
    MainBus bus;

    /// the picture bus from the PPU of the emulator
    PictureBus picture_bus;

    /// The emulator's CPU
    CPU cpu;

    /// the emulators' PPU
    PPU ppu;

    /// the virtual cartridge with ROM and mapper data
    Cartridge cartridge;

    /// a pointer to the mapper on the cartridge
    Mapper* mapper;

    /// the 2 controllers on the emulator
    Controller controller1, controller2;

    /// Skip DMA cycle and perform a DMA copy.
    void DMA(uint8_t page);

public:
    /// Initialize a new emulator with a path to a ROM file.
    ///
    /// @param rom_path the path to the ROM for the emulator to run
    ///
    Emulator(std::string rom_path);

    /// Create a new emulator as a copy of another emulator state
    ///
    /// @param emulator the emulator to copy the state from
    ///
    Emulator(Emulator* emulator);

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
    void reset();

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

};

#endif // EMULATOR_H
