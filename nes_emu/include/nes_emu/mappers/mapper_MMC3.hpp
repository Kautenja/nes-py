//  Program:      nes-py
//  File:         mapper_MMC3.hpp
//  Description:  An implementation of the MMC3 mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef MAPPERMMC3_HPP
#define MAPPERMMC3_HPP

#include <array>
#include "nes_emu/common.hpp"
#include "nes_emu/mapper.hpp"
#include "nes_emu/mapper_bank.hpp"

namespace NES {

class MapperMMC3 : public Mapper {
 private:
    /// Number of low A12 observations required before a rising edge clocks.
    static const unsigned int A12_LOW_FILTER_OBSERVATIONS = 8;
    /// The selected MMC3 bank register index.
    NES_Byte bank_select;
    /// The eight MMC3 bank registers.
    std::array<NES_Byte, 8> bank_registers;
    /// Whether PRG mode swaps the fixed second-last bank to $8000.
    bool prg_mode;
    /// Whether CHR mode maps 1 KiB banks at $0000-$0fff.
    bool chr_inversion;
    /// Whether PRG RAM is enabled by the MMC3 protect register.
    bool prg_ram_enabled;
    /// Switchable/fixed 8 KiB PRG windows.
    std::array<MapperBank::BankWindow, 4> prg_windows;
    /// 1 KiB CHR ROM windows.
    std::array<MapperBank::BankWindow, 8> chr_windows;
    /// Optional writable CHR RAM.
    MapperBank::CHRMemory chr_memory;
    /// IRQ latch written through $c000.
    NES_Byte irq_latch;
    /// Current IRQ counter.
    NES_Byte irq_counter;
    /// Whether the counter should reload on the next filtered A12 edge.
    bool irq_reload;
    /// Whether filtered counter zero events request IRQs.
    bool irq_enabled;
    /// Last observed filtered PPU A12 level.
    bool last_ppu_a12;
    /// Count of consecutive low-A12 PPU address observations.
    unsigned int ppu_a12_low_observations;

    /// Recalculate all PRG windows from current MMC3 PRG registers/mode.
    void calculatePRGWindows();

    /// Recalculate all CHR windows from current MMC3 CHR registers/mode.
    void calculateCHRWindows();

    /// Clock the MMC3 IRQ counter once after a filtered A12 rising edge.
    void clockIRQCounter();

 public:
    /// Create a new mapper with a cartridge.
    ///
    /// @param cart a reference to a cartridge for the mapper to access
    ///
    explicit MapperMMC3(Cartridge* cart);

    /// Return a copy of this mapper and its current state.
    inline std::unique_ptr<Mapper> clone() const {
        return std::unique_ptr<Mapper>(new MapperMMC3(*this));
    }

    /// Read a byte from the PRG ROM.
    NES_Byte readPRG(NES_Address address);

    /// Write a byte to an MMC3 register.
    void writePRG(NES_Address address, NES_Byte value);

    /// Read a byte from the CHR ROM/RAM.
    NES_Byte readCHR(NES_Address address);

    /// Return a direct 1 KiB CHR read page for PPU hot paths.
    const NES_Byte* getDirectCHRReadPage(NES_Address page_base);

    /// Write a byte to CHR RAM when present.
    void writeCHR(NES_Address address, NES_Byte value);

    /// Read a byte from mapper-owned PRG RAM in $6000-$7fff.
    NES_Byte readPRGRAM(NES_Address address);

    /// Write a byte to mapper-owned PRG RAM in $6000-$7fff.
    void writePRGRAM(NES_Address address, NES_Byte value);

    /// Return a pointer into PRG RAM for DMA reads, or null when unavailable.
    const NES_Byte* getPRGRAMPointer(NES_Address address);

    /// Observe a PPU address for MMC3 A12 IRQ edge detection.
    void onPPUAddress(NES_Address address);

    /// Return true because MMC3 IRQs are clocked by PPU A12 edges.
    inline bool observesPPUAddresses() const { return true; }

    /// MMC3 A12 edge observation is compatible with direct CHR reads.
    inline bool allowsDirectCHRReadWithPPUAddressObservations() const {
        return true;
    }

    /// MMC3 observes PPU addresses during tile-row fetches, not readCHR().
    inline bool allowsBackgroundTileCacheWithPPUAddressObservations() const {
        return true;
    }

    /// MMC3 IRQ timing depends on sprite-fetch A12 edges.
    inline bool requiresPPUSpriteFetchAddressObservations() const {
        return true;
    }

    /// Return true when a PRG-space write changes direct PRG read pages.
    bool invalidatesDirectPRGReadPagesOnWrite(
        NES_Address address,
        NES_Byte value
    ) const;

    /// Return true when a PRG-space write changes direct CHR read pages.
    bool invalidatesDirectCHRReadPagesOnWrite(
        NES_Address address,
        NES_Byte value
    ) const;
};

}  // namespace NES

#endif  // MAPPERMMC3_HPP
