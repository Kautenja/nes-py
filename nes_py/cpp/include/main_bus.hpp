//  Program:      nes-py
//  File:         main_bus.hpp
//  Description:  This class houses the main bus data for the NES
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef MAIN_BUS_H
#define MAIN_BUS_H

#include <vector>
#include <map>
#include <functional>
#include "cartridge.hpp"
#include "mapper.hpp"

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

/// The main bus for data to travel along the NES hardware
class MainBus {
private:
    /// The RAM on the main bus
    std::vector<uint8_t> m_RAM;
    /// The extended RAM (if the mapper has extended RAM)
    std::vector<uint8_t> m_extRAM;
    /// a pointer to the mapper on the cartridge
    Mapper* m_mapper;
    /// a map of IO registers to callback methods for writes
    std::map<IORegisters, std::function<void(uint8_t)>> m_writeCallbacks;
    /// a map of IO registers to callback methods for reads
    std::map<IORegisters, std::function<uint8_t(void)>> m_readCallbacks;

public:
    /// Initialize a new main bus.
    MainBus() : m_RAM(0x800, 0), m_mapper(nullptr) { };

    /// Return a 8-bit pointer to the RAM buffer's first address.
    ///
    /// @return a 8-bit pointer to the RAM buffer's first address
    ///
    uint8_t* get_memory_buffer() { return &m_RAM.front(); };

    /// Read a byte from an address on the RAM.
    ///
    /// @param addr the 16-bit address of the byte to read in the RAM
    ///
    /// @return the byte located at the given address
    ///
    uint8_t read(uint16_t addr);

    /// Write a byte to an address in the RAM.
    ///
    /// @param addr the 16-bit address to write the byte to in RAM
    /// @param value the byte to write to the given address
    ///
    void write(uint16_t addr, uint8_t value);

    /// Set the mapper pointer to a new value.
    ///
    /// @param mapper the new mapper pointer for the bus to use
    ///
    bool set_mapper(Mapper* mapper);

    /// Set a callback for when writes occur.
    bool set_write_callback(IORegisters reg, std::function<void(uint8_t)> callback);

    /// Set a callback for when reads occur.
    bool set_read_callback(IORegisters reg, std::function<uint8_t(void)> callback);

    /// Return a pointer to the page in memory.
    const uint8_t* get_page_pointer(uint8_t page);

};

#endif // MAIN_BUS_H
