//  Program:      nes-py
//  File:         mapper.cpp
//  Description:  This class provides an abstraction of an NES cartridge mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "mapper.hpp"
#include "mappers/mapper_NROM.hpp"
#include "mappers/mapper_SxROM.hpp"
#include "mappers/mapper_UxROM.hpp"
#include "mappers/mapper_CNROM.hpp"

Mapper* Mapper::create(Cartridge& cart, std::function<void(void)> mirroring_cb) {
    switch (static_cast<Mapper::Type>(cart.getMapper())) {
        case NROM:
            return new MapperNROM(cart);
        case SxROM:
            return new MapperSxROM(cart, mirroring_cb);
        case UxROM:
            return new MapperUxROM(cart);
        case CNROM:
            return new MapperCNROM(cart);
        default:
            return nullptr;
    }
}
