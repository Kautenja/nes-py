#ifndef EMULATOR_H
#define EMULATOR_H
#include <chrono>
#include "cpu.hpp"
#include "ppu.hpp"
#include "main_bus.hpp"
#include "picture_bus.hpp"
#include "controller.hpp"

using TimePoint = std::chrono::high_resolution_clock::time_point;

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
    std::unique_ptr<Mapper> mapper;

    /// the 2 controllers on the emulator
    Controller controller1, controller2;

    /// Load the ROM into the NES
    void loadRom();

    /// Skip DMA cycle and perform a DMA copy
    void DMA(Byte page);

public:
    /// Initialize a new emulator with a path to a ROM file.
    ///
    /// @param rom_path the path to the ROM for the emulator to run
    ///
    Emulator(std::string rom_path);

    /// Return a 32-bit pointer to the screen buffer's first address.
    ///
    /// @return a 32-bit pointer to the screen buffer's first address
    ///
    uint32_t* get_screen_buffer();

    /// Reset the emulator to the initial state.
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

    /// Backup the game state to the backup.
    void backup();

    /// Restore the game state from the backup.
    void restore();

    /// Read a byte from a 16-bit memory address
    ///
    /// @param address the address to read from memory
    ///
    /// @return the byte located at the given memory address
    ///
    uint8_t read_memory(uint16_t address);

    /// Write a byte to a 16-bit memory address
    ///
    /// @param address the address to write to in memory
    /// @param value the byte to write to the memory address
    ///
    void write_memory(uint16_t address, uint8_t value);

};

#endif // EMULATOR_H
