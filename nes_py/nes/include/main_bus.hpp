//  Program:      nes-py
//  File:         main_bus.hpp
//  Description:  This class houses the main bus data for the NES
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef MAIN_BUS_HPP
#define MAIN_BUS_HPP

#include <algorithm>
#include <vector>
#include <unordered_map>
#include "common.hpp"
#include "mapper.hpp"

namespace NES {

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

/// An enum functor object for calculating the hash of an enum class
/// https://stackoverflow.com/questions/18837857/cant-use-enum-class-as-unordered-map-key
struct EnumClassHash {
    template <typename T>
    std::size_t operator()(T t) const { return static_cast<std::size_t>(t); }
};

/// a type for write callback functions
typedef std::function<void(NES_Byte)> WriteCallback;
/// a map type from IORegsiters to WriteCallbacks
typedef std::unordered_map<IORegisters, WriteCallback, EnumClassHash> IORegisterToWriteCallbackMap;
/// a type for read callback functions
typedef std::function<NES_Byte(void)> ReadCallback;
/// a map type from IORegsiters to ReadCallbacks
typedef std::unordered_map<IORegisters, ReadCallback, EnumClassHash> IORegisterToReadCallbackMap;

/// The main bus for data to travel along the NES hardware
class MainBus {
 private:
    /// The RAM on the main bus
    std::vector<NES_Byte> ram;
    /// a pointer to the mapper on the cartridge
    Mapper* mapper;
    /// a map of IO registers to callback methods for writes
    IORegisterToWriteCallbackMap write_callbacks;
    /// a map of IO registers to callback methods for reads
    IORegisterToReadCallbackMap read_callbacks;

 public:
    /// Mutable main-bus state captured by backup/restore.
    struct State {
        std::vector<NES_Byte> ram;
    };

    /// Initialize a new main bus.
    MainBus() : ram(0x800, 0), mapper(nullptr) { }

    /// Return a 8-bit pointer to the RAM buffer's first address.
    ///
    /// @return a 8-bit pointer to the RAM buffer's first address
    ///
    inline NES_Byte* get_memory_buffer() { return &ram.front(); }

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

    /// Return a copy of mutable bus state without callback or mapper wiring.
    inline State save_state() const { return {ram}; }

    /// Restore mutable bus state without changing callbacks or mapper wiring.
    inline void load_state(const State& state) {
        ram.resize(state.ram.size());
        std::copy(state.ram.begin(), state.ram.end(), ram.begin());
    }

    /// Set a callback for when writes occur.
    inline void set_write_callback(IORegisters reg, WriteCallback callback) {
        write_callbacks.insert({reg, callback});
    }

    /// Set a callback for when reads occur.
    inline void set_read_callback(IORegisters reg, ReadCallback callback) {
        read_callbacks.insert({reg, callback});
    }

    /// Return a pointer to the page in memory.
    const NES_Byte* get_page_pointer(NES_Byte page);
};

}  // namespace NES

#endif  // MAIN_BUS_HPP
