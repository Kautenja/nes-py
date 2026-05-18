//  Program:      nes-py
//  File:         mapper_bank.hpp
//  Description:  Small helpers for mapper PRG/CHR bank selection
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef MAPPER_BANK_HPP
#define MAPPER_BANK_HPP

#include <cstddef>
#include <vector>
#include "nes_emu/common.hpp"
#include "nes_emu/cartridge.hpp"

namespace NES {
namespace MapperBank {

/// Return the number of complete banks in a byte-sized memory region.
inline std::size_t bankCount(
    std::size_t memory_size,
    std::size_t bank_size
) {
    if (bank_size == 0)
        return 0;
    return memory_size / bank_size;
}

/// Select a bank safely, aligning multi-bank windows before wrapping.
inline std::size_t maskBankSelect(
    std::size_t bank,
    std::size_t bank_count,
    std::size_t bank_span = 1
) {
    if (bank_count == 0)
        return 0;
    if (bank_span == 0)
        bank_span = 1;
    if (bank_count < bank_span)
        return 0;

    const std::size_t window_count = bank_count / bank_span;
    if (window_count == 0)
        return 0;
    return ((bank / bank_span) % window_count) * bank_span;
}

/// Resolve a mapper register write through optional bus-conflict behavior.
inline NES_Byte resolveBusConflict(
    bool enabled,
    NES_Byte write_value,
    NES_Byte visible_value
) {
    return enabled ? (write_value & visible_value) : write_value;
}

/// A fixed-size PRG or CHR window into a ROM/RAM bank collection.
class BankWindow {
 private:
    std::size_t base_offset;
    std::size_t mapped_size;
    std::size_t selected_bank;

    inline void setBank(
        std::size_t memory_size,
        std::size_t bank_size,
        std::size_t window_size,
        std::size_t bank
    ) {
        mapped_size = window_size == 0 ? 1 : window_size;
        selected_bank = maskBankSelect(
            bank,
            bankCount(memory_size, bank_size)
        );
        base_offset = selected_bank * bank_size;
    }

 public:
    /// Create a bank window pointing to the first byte.
    BankWindow() : base_offset(0), mapped_size(1), selected_bank(0) { }

    /// Return the selected bank index at this window's bank granularity.
    inline std::size_t bank() const { return selected_bank; }

    /// Return the byte offset of the selected bank window.
    inline std::size_t base() const { return base_offset; }

    /// Select a single bank, optionally aligning to a multi-bank span.
    inline void selectBank(
        std::size_t memory_size,
        std::size_t bank_size,
        std::size_t bank,
        std::size_t bank_span = 1
    ) {
        mapped_size = bank_size == 0 ? 1 : bank_size;
        selected_bank = maskBankSelect(
            bank,
            bankCount(memory_size, bank_size),
            bank_span
        );
        base_offset = selected_bank * bank_size;
    }

    /// Select a bank by already-resolved index.
    inline void selectBankIndex(
        std::size_t memory_size,
        std::size_t bank_size,
        std::size_t bank
    ) {
        setBank(memory_size, bank_size, bank_size, bank);
    }

    /// Select a wider window whose register is expressed in bank units.
    inline void selectWindow(
        std::size_t memory_size,
        std::size_t bank_size,
        std::size_t window_size,
        std::size_t bank
    ) {
        const std::size_t span = bank_size == 0 ?
            1 :
            (window_size + bank_size - 1) / bank_size;
        mapped_size = window_size == 0 ? bank_size : window_size;
        selected_bank = maskBankSelect(
            bank,
            bankCount(memory_size, bank_size),
            span
        );
        base_offset = selected_bank * bank_size;
    }

    /// Select the first complete bank.
    inline void selectFirst(std::size_t memory_size, std::size_t bank_size) {
        selectBankIndex(memory_size, bank_size, 0);
    }

    /// Select the final complete bank.
    inline void selectFinal(std::size_t memory_size, std::size_t bank_size) {
        const std::size_t count = bankCount(memory_size, bank_size);
        selectBankIndex(memory_size, bank_size, count == 0 ? 0 : count - 1);
    }

    /// Select the final complete window.
    inline void selectFinalWindow(
        std::size_t memory_size,
        std::size_t bank_size,
        std::size_t window_size
    ) {
        const std::size_t count = bankCount(memory_size, bank_size);
        const std::size_t span = bank_size == 0 ?
            1 :
            (window_size + bank_size - 1) / bank_size;
        const std::size_t bank = count <= span ? 0 : count - span;
        setBank(memory_size, bank_size, window_size, bank);
    }

    /// Resolve a CPU/PPU address into the selected memory byte offset.
    inline std::size_t offset(
        NES_Address address,
        NES_Address window_base
    ) const {
        return base_offset + ((address - window_base) % mapped_size);
    }

    /// Read through this bank window.
    inline NES_Byte read(
        const std::vector<NES_Byte>& memory,
        NES_Address address,
        NES_Address window_base
    ) const {
        if (memory.empty())
            return 0;
        return memory[offset(address, window_base) % memory.size()];
    }

    /// Return a direct pointer into this window when a contiguous read fits.
    inline const NES_Byte* readPointer(
        const std::vector<NES_Byte>& memory,
        NES_Address address,
        NES_Address window_base,
        std::size_t contiguous_size
    ) const {
        if (memory.empty())
            return nullptr;
        const std::size_t window_offset = (address - window_base) % mapped_size;
        const std::size_t memory_offset = base_offset + window_offset;
        if (
            window_offset + contiguous_size > mapped_size ||
            memory_offset + contiguous_size > memory.size()
        ) {
            return nullptr;
        }
        return &memory[memory_offset];
    }
};

/// Mapper-owned CHR RAM with explicit ROM/RAM behavior.
class CHRMemory {
 private:
    bool ram_backed;
    std::vector<NES_Byte> ram;

 public:
    /// Create empty CHR memory.
    CHRMemory() : ram_backed(false), ram() { }

    /// Create CHR memory from cartridge metadata.
    explicit CHRMemory(Cartridge* cartridge) :
        ram_backed(
            cartridge != nullptr &&
            cartridge->getVROM().empty() &&
            cartridge->getCHRRAMSize() > 0
        ),
        ram(ram_backed ? cartridge->getCHRRAMSize() : 0, 0) { }

    /// Return whether this cartridge uses writable CHR RAM.
    inline bool usesRAM() const { return ram_backed; }

    /// Return the CHR RAM byte size.
    inline std::size_t size() const { return ram.size(); }

    /// Read CHR RAM, mirroring addresses through the available RAM.
    inline NES_Byte read(NES_Address address) const {
        if (ram.empty())
            return 0;
        return ram[address % ram.size()];
    }

    /// Write CHR RAM, mirroring addresses through the available RAM.
    inline void write(NES_Address address, NES_Byte value) {
        if (!ram.empty())
            ram[address % ram.size()] = value;
    }

    /// Return a direct pointer into CHR RAM when a contiguous read fits.
    inline const NES_Byte* readPointer(
        NES_Address address,
        std::size_t contiguous_size
    ) const {
        if (ram.empty())
            return nullptr;
        const std::size_t offset = address % ram.size();
        if (offset + contiguous_size > ram.size())
            return nullptr;
        return &ram[offset];
    }
};

}  // namespace MapperBank
}  // namespace NES

#endif  // MAPPER_BANK_HPP
