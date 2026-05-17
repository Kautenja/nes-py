//  Program:      nes-py
//  File:         mapper_FME7.hpp
//  Description:  An implementation of the Sunsoft FME-7 / 5B mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef MAPPERFME7_HPP
#define MAPPERFME7_HPP

#include <array>
#include <cstddef>
#include <vector>
#include "nes_emu/common.hpp"
#include "nes_emu/mapper.hpp"
#include "nes_emu/mapper_bank.hpp"

namespace NES {

class MapperFME7 : public Mapper {
 private:
    static const std::size_t PRG_WINDOW_SIZE = 0x2000;
    static const std::size_t CHR_WINDOW_SIZE = 0x0400;
    static const std::size_t PRG_WINDOW_COUNT = 3;
    static const std::size_t CHR_WINDOW_COUNT = 8;
    static const std::size_t AUDIO_REGISTER_COUNT = 16;

    /// Currently selected FME-7 command register.
    NES_Byte selected_command;
    /// Raw command 8 state for CPU $6000-$7fff mapping.
    NES_Byte prg_6000_control;
    /// PRG ROM bank registers for $8000, $a000, and $c000.
    std::array<NES_Byte, PRG_WINDOW_COUNT> prg_registers;
    /// CHR bank registers for eight 1 KiB PPU windows.
    std::array<NES_Byte, CHR_WINDOW_COUNT> chr_registers;
    /// Bankable PRG ROM/RAM window mapped at CPU $6000-$7fff.
    MapperBank::BankWindow prg_6000_window;
    /// Bankable PRG ROM windows mapped at CPU $8000-$dfff.
    std::array<MapperBank::BankWindow, PRG_WINDOW_COUNT> prg_windows;
    /// Fixed final PRG ROM window mapped at CPU $e000-$ffff.
    MapperBank::BankWindow fixed_prg;
    /// Bankable CHR ROM/RAM windows mapped in 1 KiB PPU pages.
    std::array<MapperBank::BankWindow, CHR_WINDOW_COUNT> chr_windows;
    /// Optional writable CHR RAM used when the cartridge has no CHR ROM.
    std::vector<NES_Byte> chr_ram;
    /// Whether the IRQ counter decrements on CPU cycles.
    bool irq_counter_enabled;
    /// Whether an IRQ is requested when the counter wraps.
    bool irq_enabled;
    /// Whether an IRQ has been requested and not yet acknowledged.
    bool irq_pending;
    /// The 16-bit CPU-cycle IRQ counter.
    std::uint16_t irq_counter;
    /// Sunsoft 5B audio address latch; audio mixing is not implemented.
    NES_Byte audio_register_select;
    /// Stored Sunsoft 5B audio register writes for state preservation.
    std::array<NES_Byte, AUDIO_REGISTER_COUNT> audio_registers;

    /// Return true when command 8 selects PRG RAM at $6000-$7fff.
    bool prg6000SelectsRAM() const;

    /// Return true when command 8 enables PRG RAM reads and writes.
    bool prg6000RAMEnabled() const;

    /// Recalculate the CPU $6000-$7fff bank window.
    void updatePRG6000Window();

    /// Recalculate one CPU $8000-$dfff PRG ROM bank window.
    void updatePRGWindow(std::size_t index);

    /// Recalculate one PPU 1 KiB CHR bank window.
    void updateCHRWindow(std::size_t index);

    /// Apply the currently selected command to a parameter write.
    void applyCommand(NES_Byte value);

 public:
    /// Create a new mapper with a cartridge.
    ///
    /// @param cart a reference to a cartridge for the mapper to access
    ///
    explicit MapperFME7(Cartridge* cart);

    /// Return a copy of this mapper and its current state.
    inline std::unique_ptr<Mapper> clone() const {
        return std::unique_ptr<Mapper>(new MapperFME7(*this));
    }

    /// Read a byte from mapper-controlled PRG RAM/ROM at $6000-$7fff.
    NES_Byte readPRGRAM(NES_Address address);

    /// Write a byte to mapper-controlled PRG RAM at $6000-$7fff.
    void writePRGRAM(NES_Address address, NES_Byte value);

    /// Return a pointer into PRG RAM for DMA reads, or null when unavailable.
    const NES_Byte* getPRGRAMPointer(NES_Address address);

    /// Read a byte from PRG ROM.
    NES_Byte readPRG(NES_Address address);

    /// Write a byte to a mapper control register.
    void writePRG(NES_Address address, NES_Byte value);

    /// Read a byte from CHR ROM/RAM.
    NES_Byte readCHR(NES_Address address);

    /// Write a byte to CHR RAM.
    void writeCHR(NES_Address address, NES_Byte value);

    /// Observe one CPU cycle for the FME-7 IRQ counter.
    void onCPUCycle();

    /// Return true because FME-7 can enable its IRQ counter at runtime.
    inline bool observesCPUCycles() const { return true; }
};

}  // namespace NES

#endif  // MAPPERFME7_HPP
