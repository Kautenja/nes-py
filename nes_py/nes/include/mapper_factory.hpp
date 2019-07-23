//  Program:      nes-py
//  File:         mapper.hpp
//  Description:  An abstract factory for mappers
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef MAPPER_FACTORY_HPP
#define MAPPER_FACTORY_HPP

#include "mapper.hpp"
#include "mappers/mapper_NROM.hpp"
#include "mappers/mapper_SxROM.hpp"
#include "mappers/mapper_UxROM.hpp"
#include "mappers/mapper_CNROM.hpp"

namespace NES {

/// an enumeration of mapper IDs
enum class MapperID : NES_Byte {
    NROM  = 0,
    SxROM = 1,
    UxROM = 2,
    CNROM = 3,
};

/// Create a mapper for the given cartridge with optional callback function
///
/// @param game the cartridge to initialize a mapper for
/// @param callback the callback function for the mapper (if necessary)
///
Mapper* MapperFactory(Cartridge* game, std::function<void(void)> callback) {
    switch (static_cast<MapperID>(game->getMapper())) {
        case MapperID::NROM:
            return new MapperNROM(game);
        case MapperID::SxROM:
            return new MapperSxROM(game, callback);
        case MapperID::UxROM:
            return new MapperUxROM(game);
        case MapperID::CNROM:
            return new MapperCNROM(game);
        default:
            return nullptr;
    }
}

}  // namespace NES

#endif  // MAPPER_FACTORY_HPP
