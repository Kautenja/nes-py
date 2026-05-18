//  Program:      nes-py
//  File:         emulator.hpp
//  Description:  This class houses the logic and data for an NES emulator
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef EMULATOR_HPP
#define EMULATOR_HPP

#include <cstddef>
#include <memory>
#include <string>
#include "nes_emu/common.hpp"
#include "nes_emu/cartridge.hpp"
#include "nes_emu/controller.hpp"
#include "nes_emu/cpu.hpp"
#include "nes_emu/ppu.hpp"
#include "nes_emu/main_bus.hpp"
#include "nes_emu/picture_bus.hpp"
#include "nes_emu/mapper.hpp"

namespace NES {

/// An NES Emulator and Gymnasium interface
class Emulator {
    friend struct EmulatorInspector;

 private:
    /// The number of cycles in 1 frame
    static const int CYCLES_PER_FRAME = 29781;
    /// the virtual cartridge with ROM and mapper data
    Cartridge cartridge;
    /// the 2 controllers on the emulator
    Controller controllers[2];

    /// the main data bus of the emulator
    MainBus bus;
    /// the picture bus from the PPU of the emulator
    PictureBus picture_bus;
    /// The emulator's CPU
    CPU cpu;
    /// the emulators' PPU
    PPU ppu;
    /// the active mapper on the loaded cartridge
    std::unique_ptr<Mapper> mapper;

    /// the main data bus of the emulator
    MainBus::State backup_bus;
    /// the picture bus from the PPU of the emulator
    PictureBus::State backup_picture_bus;
    /// The emulator's CPU
    CPU backup_cpu;
    /// the emulators' PPU
    PPU::Snapshot backup_ppu;
    /// the mapper state captured in the backup snapshot
    std::unique_ptr<Mapper> backup_mapper;
    /// cached mapper capability flag for CPU-cycle hook dispatch
    bool mapper_observes_cpu_cycles;

    /// Reconnect mapper pointers and callbacks after initialization/restore.
    void wire_mapper();

    /// Refresh picture-bus mirroring if the mapper changed it.
    void synchronize_mapper_mirroring();

    /// Advance PPU and mapper timing for one CPU cycle.
    void advance_ppu_timing_cycle();

    /// Perform a frame using the original CPU-cycle-by-cycle path.
    void step_cycle_by_cycle();

    /// Perform a frame using instruction-level CPU batching.
    void step_instruction_batched();

    /// Return true when the active mapper permits instruction batching.
    inline bool can_batch_cpu_instructions() const {
        return !mapper_observes_cpu_cycles;
    }

 public:
    /// The width of the NES screen in pixels
    static const int WIDTH = SCANLINE_VISIBLE_DOTS;
    /// The height of the NES screen in pixels
    static const int HEIGHT = VISIBLE_SCANLINES;

    /// Initialize a new emulator with a path to a ROM file.
    ///
    /// @param rom_path the path to the ROM for the emulator to run
    ///
    explicit Emulator(std::string rom_path);

    /// Return a 32-bit pointer to the screen buffer's first address.
    ///
    /// @return a 32-bit pointer to the screen buffer's first address
    ///
    inline NES_Pixel* get_screen_buffer() { return ppu.get_screen_buffer(); }

    /// Return a 8-bit pointer to the RAM buffer's first address.
    ///
    /// @return a 8-bit pointer to the RAM buffer's first address
    ///
    inline NES_Byte* get_memory_buffer() { return bus.get_memory_buffer(); }

    /// Return a pointer to a controller port
    ///
    /// @param port the port of the controller to return the pointer to
    /// @return a pointer to the byte buffer for the controller state
    ///
    inline NES_Byte* get_controller(int port) {
        return controllers[port].get_joypad_buffer();
    }

    /// Load the ROM into the NES.
    inline void reset() { cpu.reset(bus); ppu.reset(); synchronize_mapper_mirroring(); }

    /// Perform a step on the emulator, i.e., a single frame.
    void step();

    /// Create a backup state on the emulator.
    void backup();

    /// Restore the backup state on the emulator.
    void restore();

    /// Return the loaded cartridge mapper number.
    inline int get_mapper_number() { return cartridge.getMapper(); }

    /// Return the loaded cartridge submapper number.
    inline int get_submapper() { return cartridge.getSubmapper(); }

    /// Return the PRG ROM size in bytes.
    inline std::size_t get_prg_rom_size() { return cartridge.getROM().size(); }

    /// Return the CHR ROM size in bytes.
    inline std::size_t get_chr_rom_size() { return cartridge.getVROM().size(); }

    /// Return true when this cartridge uses CHR RAM instead of CHR ROM.
    inline bool has_chr_ram() { return cartridge.getCHRRAMSize() > 0; }

    /// Return the active mapper's name table mirroring mode.
    inline int get_name_table_mirroring() {
        return mapper->getNameTableMirroring();
    }
};

}  // namespace NES

#endif  // EMULATOR_HPP
