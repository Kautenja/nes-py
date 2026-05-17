//  Program:      nes-py
//  File:         main_bus.hpp
//  Description:  This class houses the main bus data for the NES
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef MAIN_BUS_HPP
#define MAIN_BUS_HPP

#include <array>
#include <functional>
#include <utility>
#include "common.hpp"
#include "mapper.hpp"

namespace NES {

class Controller;
class CPU;
class PictureBus;
class PPU;

/// The IO registers on the main bus
enum IORegisters {
    PPUCTRL = 0x2000,
    PPUMASK,
    PPUSTATUS,
    OAMADDR,
    OAMDATA,
    PPUSCROL,
    PPUADDR,
    PPUDATA,
    OAMDMA = 0x4014,
    JOY1 = 0x4016,
    JOY2 = 0x4017,
};

/// a type for write callback functions
typedef std::function<void(NES_Byte)> WriteCallback;
/// a type for read callback functions
typedef std::function<NES_Byte(void)> ReadCallback;

/// The main bus for data to travel along the NES hardware
class MainBus {
 private:
    /// Number of bytes in mirrored CPU work RAM.
    static const std::size_t RAM_SIZE = 0x800;
    /// Number of PPU registers visible on the CPU bus.
    static const std::size_t PPU_REGISTER_COUNT = 8;
    /// Number of callback-capable I/O registers from $4014 through $4017.
    static const std::size_t IO_REGISTER_COUNT = 4;
    /// The RAM on the main bus
    std::array<NES_Byte, RAM_SIZE> ram;
    /// a pointer to the mapper on the cartridge
    Mapper* mapper;
    /// Direct CPU device pointer for DMA cycle penalties.
    CPU* cpu;
    /// Direct PPU device pointer for register dispatch.
    PPU* ppu;
    /// Direct picture-bus pointer for PPUDATA dispatch.
    PictureBus* picture_bus;
    /// Direct controller pointer for joypad register dispatch.
    Controller* controllers;
    /// Direct callback slots for mirrored PPU register writes.
    std::array<WriteCallback, PPU_REGISTER_COUNT> ppu_write_callbacks;
    /// Direct callback slots for mirrored PPU register reads.
    std::array<ReadCallback, PPU_REGISTER_COUNT> ppu_read_callbacks;
    /// Direct callback slots for $4014-$4017 register writes.
    std::array<WriteCallback, IO_REGISTER_COUNT> io_write_callbacks;
    /// Direct callback slots for $4014-$4017 register reads.
    std::array<ReadCallback, IO_REGISTER_COUNT> io_read_callbacks;

 public:
    /// Mutable main-bus state captured by backup/restore.
    struct State {
        std::array<NES_Byte, RAM_SIZE> ram{};
    };

    /// Initialize a new main bus.
    MainBus() :
        ram(),
        mapper(nullptr),
        cpu(nullptr),
        ppu(nullptr),
        picture_bus(nullptr),
        controllers(nullptr) { }

    /// Return a 8-bit pointer to the RAM buffer's first address.
    ///
    /// @return a 8-bit pointer to the RAM buffer's first address
    ///
    inline NES_Byte* get_memory_buffer() { return ram.data(); }

    /// Read a byte from an address on the RAM.
    ///
    /// @param address the 16-bit address of the byte to read in the RAM
    ///
    /// @return the byte located at the given address
    ///
    NES_Byte read(NES_Address address);

    /// Write a byte to an address in the RAM.
    ///
    /// @param address the 16-bit address to write the byte to in RAM
    /// @param value the byte to write to the given address
    ///
    void write(NES_Address address, NES_Byte value);

    /// Set the mapper pointer to a new value.
    ///
    /// @param mapper the new mapper pointer for the bus to use
    ///
    inline void set_mapper(Mapper* mapper) { this->mapper = mapper; }

    /// Connect native device pointers for direct hot-path register dispatch.
    inline void connect_devices(
        CPU* cpu,
        PPU* ppu,
        PictureBus* picture_bus,
        Controller* controllers
    ) {
        this->cpu = cpu;
        this->ppu = ppu;
        this->picture_bus = picture_bus;
        this->controllers = controllers;
    }

    /// Return a copy of mutable bus state without callback or mapper wiring.
    inline State save_state() const { return {ram}; }

    /// Restore mutable bus state without changing callbacks or mapper wiring.
    inline void load_state(const State& state) { ram = state.ram; }

    /// Set a callback for when writes occur.
    inline void set_write_callback(IORegisters reg, WriteCallback callback) {
        if (reg >= PPUCTRL && reg <= PPUDATA) {
            ppu_write_callbacks[reg - PPUCTRL] = std::move(callback);
        } else if (reg >= OAMDMA && reg <= JOY2) {
            io_write_callbacks[reg - OAMDMA] = std::move(callback);
        }
    }

    /// Set a callback for when reads occur.
    inline void set_read_callback(IORegisters reg, ReadCallback callback) {
        if (reg >= PPUCTRL && reg <= PPUDATA) {
            ppu_read_callbacks[reg - PPUCTRL] = std::move(callback);
        } else if (reg >= OAMDMA && reg <= JOY2) {
            io_read_callbacks[reg - OAMDMA] = std::move(callback);
        }
    }

    /// Return a pointer to the page in memory.
    const NES_Byte* get_page_pointer(NES_Byte page);
};

}  // namespace NES

#endif  // MAIN_BUS_HPP
