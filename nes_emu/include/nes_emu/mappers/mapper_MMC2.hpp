//  Program:      nes-py
//  File:         mapper_MMC2.hpp
//  Description:  An implementation of the MMC2 mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef MAPPERMMC2_HPP
#define MAPPERMMC2_HPP

#include "nes_emu/common.hpp"
#include "nes_emu/mapper.hpp"
#include "nes_emu/mapper_bank.hpp"

namespace NES {

class MapperMMC2 : public Mapper {
 private:
    enum PendingLatch {
        LATCH_NONE,
        LATCH_LEFT_FD,
        LATCH_LEFT_FE,
        LATCH_RIGHT_FD,
        LATCH_RIGHT_FE,
    };

    /// Switchable 8 KiB PRG window mapped at $8000-$9fff.
    MapperBank::BankWindow switchable_prg;
    /// Fixed PRG window mapped at $a000-$bfff.
    MapperBank::BankWindow fixed_prg_a;
    /// Fixed PRG window mapped at $c000-$dfff.
    MapperBank::BankWindow fixed_prg_b;
    /// Fixed PRG window mapped at $e000-$ffff.
    MapperBank::BankWindow fixed_prg_c;
    /// Left 4 KiB CHR window mapped at PPU $0000-$0fff.
    MapperBank::BankWindow left_chr;
    /// Right 4 KiB CHR window mapped at PPU $1000-$1fff.
    MapperBank::BankWindow right_chr;
    /// Optional writable CHR RAM.
    MapperBank::CHRMemory chr_memory;
    /// The selected switchable PRG bank register.
    NES_Byte register_prg;
    /// CHR bank selected when the left latch is in the FD state.
    NES_Byte register_chr_left_fd;
    /// CHR bank selected when the left latch is in the FE state.
    NES_Byte register_chr_left_fe;
    /// CHR bank selected when the right latch is in the FD state.
    NES_Byte register_chr_right_fd;
    /// CHR bank selected when the right latch is in the FE state.
    NES_Byte register_chr_right_fe;
    /// Whether the left CHR latch is in the FE state.
    bool left_latch_fe;
    /// Whether the right CHR latch is in the FE state.
    bool right_latch_fe;
    /// Latch transition staged by the pre-read PPU address observer.
    PendingLatch pending_latch;
    /// The PPU address that staged the pending latch transition.
    NES_Address pending_latch_address;

    /// Recalculate the switchable PRG window from the PRG register.
    void updatePRG();

    /// Recalculate fixed PRG windows from the cartridge size.
    void updateFixedPRG();

    /// Recalculate the left CHR window from its latch and registers.
    void updateLeftCHR();

    /// Recalculate the right CHR window from its latch and registers.
    void updateRightCHR();

    /// Apply a staged latch transition after a matching CHR read.
    void applyPendingLatch(NES_Address address);

 public:
    /// Create a new mapper with a cartridge.
    ///
    /// @param cart a reference to a cartridge for the mapper to access
    ///
    explicit MapperMMC2(Cartridge* cart);

    /// Return a copy of this mapper and its current state.
    inline std::unique_ptr<Mapper> clone() const {
        return std::unique_ptr<Mapper>(new MapperMMC2(*this));
    }

    /// Read a byte from PRG ROM.
    ///
    /// @param address the 16-bit address of the byte to read
    /// @return the byte located at the given address in PRG ROM
    ///
    NES_Byte readPRG(NES_Address address);

    /// Write a byte to a mapper register.
    ///
    /// @param address the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    void writePRG(NES_Address address, NES_Byte value);

    /// Read a byte from CHR ROM or CHR RAM.
    ///
    /// @param address the 16-bit address of the byte to read
    /// @return the byte located at the given address in CHR memory
    ///
    NES_Byte readCHR(NES_Address address);

    /// Write a byte to CHR RAM when present.
    ///
    /// @param address the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    void writeCHR(NES_Address address, NES_Byte value);

    /// Observe a PPU address before a picture-bus access.
    void onPPUAddress(NES_Address address);

    /// Return true because MMC2 CHR latches are PPU-address driven.
    inline bool observesPPUAddresses() const { return true; }
};

}  // namespace NES

#endif  // MAPPERMMC2_HPP
