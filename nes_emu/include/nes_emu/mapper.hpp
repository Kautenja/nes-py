//  Program:      nes-py
//  File:         mapper.hpp
//  Description:  This class provides an abstraction of an NES cartridge mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef MAPPER_HPP
#define MAPPER_HPP

#include <cstddef>
#include <functional>
#include <memory>
#include <vector>
#include "nes_emu/common.hpp"
#include "nes_emu/cartridge.hpp"
#include "nes_emu/mapper_bank.hpp"

namespace NES {

/// Mirroring modes supported by the NES
enum NameTableMirroring {
    HORIZONTAL  = 0,
    VERTICAL    = 1,
    FOUR_SCREEN  = 8,
    ONE_SCREEN_LOWER,
    ONE_SCREEN_HIGHER,
};

/// An abstraction of a general hardware mapper for different NES cartridges
class Mapper {
 protected:
    /// The cartridge this mapper associates with
    Cartridge* cartridge;
    /// Directly-addressable PRG RAM owned by the mapper.
    std::vector<NES_Byte> prg_ram;
    /// Whether PRG RAM writes are enabled.
    bool prg_ram_writable;
    /// Callback used by mapper IRQ-capable hardware.
    std::function<void(void)> irq_callback;
    /// The current mapper-controlled name table mirroring mode.
    NameTableMirroring name_table_mirroring;
    /// Whether the mapper changed mirroring since the last synchronization.
    bool name_table_mirroring_changed;

    /// Resize mapper-owned PRG RAM.
    inline void resizePRGRAM(std::size_t size) { prg_ram.resize(size); }

    /// Enable or disable writes to mapper-owned PRG RAM.
    inline void setPRGRAMWritable(bool writable) {
        prg_ram_writable = writable;
    }

    /// Request a CPU IRQ through the emulator-provided callback.
    inline void requestIRQ() {
        if (irq_callback)
            irq_callback();
    }

    /// Set the mapper-controlled mirroring mode.
    inline void setNameTableMirroring(NameTableMirroring mirroring) {
        if (name_table_mirroring != mirroring) {
            name_table_mirroring = mirroring;
            name_table_mirroring_changed = true;
        }
    }

 public:
    /// Create a new mapper with a cartridge and given type.
    ///
    /// @param game a reference to a cartridge for the mapper to access
    ///
    explicit Mapper(Cartridge* game) :
        cartridge(game),
        prg_ram(game == nullptr ? 0 : game->getPRGMemorySize(), 0),
        prg_ram_writable(true),
        name_table_mirroring(game == nullptr ?
            HORIZONTAL :
            static_cast<NameTableMirroring>(game->getNameTableMirroring())),
        name_table_mirroring_changed(true) { }

    /// Copy mapper state without copying emulator callback wiring.
    Mapper(const Mapper& other) :
        cartridge(other.cartridge),
        prg_ram(other.prg_ram),
        prg_ram_writable(other.prg_ram_writable),
        name_table_mirroring(other.name_table_mirroring),
        name_table_mirroring_changed(other.name_table_mirroring_changed) { }

    /// Assign mapper state without copying emulator callback wiring.
    Mapper& operator=(const Mapper& other) {
        if (this != &other) {
            cartridge = other.cartridge;
            prg_ram = other.prg_ram;
            prg_ram_writable = other.prg_ram_writable;
            irq_callback = nullptr;
            name_table_mirroring = other.name_table_mirroring;
            name_table_mirroring_changed = other.name_table_mirroring_changed;
        }
        return *this;
    }

    /// Destroy this mapper.
    virtual ~Mapper() { }

    /// Return an owned copy of this mapper and its current state.
    virtual std::unique_ptr<Mapper> clone() const = 0;

    /// Set the callback used for mapper IRQ requests.
    inline void setIRQCallback(std::function<void(void)> callback) {
        irq_callback = callback;
    }

    /// Return true when the mapper changed mirroring and the picture bus
    /// should refresh its derived nametable wiring.
    inline bool hasNameTableMirroringChanged() const {
        return name_table_mirroring_changed;
    }

    /// Clear the pending mirroring synchronization flag.
    inline void clearNameTableMirroringChanged() {
        name_table_mirroring_changed = false;
    }

    /// Return the name table mirroring mode of this mapper.
    inline virtual NameTableMirroring getNameTableMirroring() {
        return name_table_mirroring;
    }

    /// Return true if this mapper has extended RAM, false otherwise.
    inline bool hasExtendedRAM() { return getPRGRAMSize() > 0; }

    /// Return the byte size of directly-addressable extended RAM.
    inline std::size_t getExtendedRAMSize() { return getPRGRAMSize(); }

    /// Return the byte size of directly-addressable PRG RAM.
    inline virtual std::size_t getPRGRAMSize() const { return prg_ram.size(); }

    /// Read a byte from the $4020-$5fff expansion area.
    inline virtual NES_Byte readExpansion(NES_Address address) {
        (void) address;
        return 0;
    }

    /// Write a byte to the $4020-$5fff expansion area.
    inline virtual void writeExpansion(NES_Address address, NES_Byte value) {
        (void) address;
        (void) value;
    }

    /// Return true when this mapper handles expansion-area accesses.
    inline virtual bool handlesExpansion(NES_Address address) const {
        (void) address;
        return false;
    }

    /// Read a byte from mapper-owned PRG RAM in $6000-$7fff.
    inline virtual NES_Byte readPRGRAM(NES_Address address) {
        std::size_t offset = address - 0x6000;
        if (offset < prg_ram.size())
            return prg_ram[offset];
        return 0;
    }

    /// Write a byte to mapper-owned PRG RAM in $6000-$7fff.
    inline virtual void writePRGRAM(NES_Address address, NES_Byte value) {
        std::size_t offset = address - 0x6000;
        if (prg_ram_writable && offset < prg_ram.size())
            prg_ram[offset] = value;
    }

    /// Return a pointer into PRG RAM for DMA reads, or null when unavailable.
    inline virtual const NES_Byte* getPRGRAMPointer(NES_Address address) {
        std::size_t offset = address - 0x6000;
        if (offset < prg_ram.size())
            return &prg_ram[offset];
        return nullptr;
    }

    /// Return a direct PRG-ROM read page for CPU reads, or null if unsafe.
    inline virtual const NES_Byte* getDirectPRGReadPage(
        NES_Address page_base
    ) {
        (void) page_base;
        return nullptr;
    }

    /// Return a direct CHR read page for PPU reads, or null if unsafe.
    inline virtual const NES_Byte* getDirectCHRReadPage(
        NES_Address page_base
    ) {
        (void) page_base;
        return nullptr;
    }

    /// Read a byte from the PRG RAM.
    ///
    /// @param address the 16-bit address of the byte to read
    /// @return the byte located at the given address in PRG RAM
    ///
    virtual NES_Byte readPRG(NES_Address address) = 0;

    /// Write a byte to an address in the PRG RAM.
    ///
    /// @param address the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    virtual void writePRG(NES_Address address, NES_Byte value) = 0;

    /// Read a byte from the CHR RAM.
    ///
    /// @param address the 16-bit address of the byte to read
    /// @return the byte located at the given address in CHR RAM
    ///
    virtual NES_Byte readCHR(NES_Address address) = 0;

    /// Write a byte to an address in the CHR RAM.
    ///
    /// @param address the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    virtual void writeCHR(NES_Address address, NES_Byte value) = 0;

    /// Observe one CPU cycle.
    inline virtual void onCPUCycle() { }

    /// Return true when this mapper needs per-CPU-cycle callbacks.
    inline virtual bool observesCPUCycles() const { return false; }

    /// Observe a PPU address before a picture-bus access.
    inline virtual void onPPUAddress(NES_Address address) {
        (void) address;
    }

    /// Return true when this mapper needs PPU address callbacks.
    inline virtual bool observesPPUAddresses() const { return false; }

    /// Observe a PPU read after data is resolved.
    inline virtual void onPPURead(NES_Address address, NES_Byte value) {
        (void) address;
        (void) value;
    }

    /// Return true when this mapper needs PPU read callbacks.
    inline virtual bool observesPPUReads() const { return false; }

    /// Observe a PPU write after data is stored.
    inline virtual void onPPUWrite(NES_Address address, NES_Byte value) {
        (void) address;
        (void) value;
    }

    /// Return true when this mapper needs PPU write callbacks.
    inline virtual bool observesPPUWrites() const { return false; }

    /// Return true when sprite pattern rows may be prefetched one scanline
    /// ahead without changing mapper-visible CHR behavior.
    inline virtual bool allowsSpriteRowPrefetch() const { return false; }

    /// Return true when this mapper may own or remap nametable data.
    inline virtual bool hasNameTableMapping() const { return false; }

    /// Return true when the mapper owns or remaps nametable data.
    inline virtual bool mapsNameTable(NES_Address address) const {
        (void) address;
        return false;
    }

    /// Read mapper-owned or mapper-remapped nametable data.
    inline virtual NES_Byte readNameTable(NES_Address address) {
        (void) address;
        return 0;
    }

    /// Write mapper-owned or mapper-remapped nametable data.
    inline virtual void writeNameTable(NES_Address address, NES_Byte value) {
        (void) address;
        (void) value;
    }

    /// Return true if writes should be resolved against visible PRG data.
    inline virtual bool hasBusConflicts() const { return false; }

    /// Resolve a mapper register write value with optional bus conflicts.
    inline virtual NES_Byte resolveBusConflict(
        NES_Address address,
        NES_Byte value
    ) {
        if (!hasBusConflicts())
            return value;
        return MapperBank::resolveBusConflict(true, value, readPRG(address));
    }
};

}  // namespace NES

#endif  // MAPPER_HPP
