//  Program:      nes-py
//  File:         picture_bus.hpp
//  Description:  This class houses picture bus data from the PPU
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef PICTURE_BUS_HPP
#define PICTURE_BUS_HPP

#include <vector>
#include <cstdlib>
#include "common.hpp"
#include "mapper.hpp"

namespace NES {

/// The bus for graphical data to travel along
class PictureBus {
 private:
    /// the VRAM on the picture bus
    std::vector<NES_Byte> ram;
    /// indexes where they start in RAM vector
    std::size_t name_tables[4] = {0, 0, 0, 0};
    /// the palette for decoding RGB tuples
    std::vector<NES_Byte> palette;
    /// a pointer to the mapper on the cartridge
    Mapper* mapper;

 public:
    /// Initialize a new picture bus.
    PictureBus() : ram(0x800), palette(0x20), mapper(nullptr) { }

    /// Read a byte from an address on the VRAM.
    ///
    /// @param address the 16-bit address of the byte to read in the VRAM
    ///
    /// @return the byte located at the given address
    ///
    NES_Byte read(NES_Address address);

    /// Write a byte to an address in the VRAM.
    ///
    /// @param address the 16-bit address to write the byte to in VRAM
    /// @param value the byte to write to the given address
    ///
    void write(NES_Address address, NES_Byte value);

    /// Set the mapper pointer to a new value.
    ///
    /// @param mapper the new mapper pointer for the bus to use
    ///
    inline void set_mapper(Mapper *mapper) {
        this->mapper = mapper; update_mirroring();
    }

    /// Read a color index from the palette.
    ///
    /// @param address the address of the palette color
    ///
    /// @return the index of the RGB tuple in the color array
    ///
    inline NES_Byte read_palette(NES_Byte address) {
        return palette[address];
    }

    /// Update the mirroring and name table from the mapper.
    void update_mirroring();
};

}  // namespace NES

#endif  // PICTURE_BUS_HPP
