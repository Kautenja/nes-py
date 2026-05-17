//  Program:      nes-py
//  File:         mapper_factory.cpp
//  Description:  Registration-based factory for native NES mappers
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "nes_emu/mapper_factory.hpp"
#include "nes_emu/mappers/mapper_NROM.hpp"
#include "nes_emu/mappers/mapper_SxROM.hpp"
#include "nes_emu/mappers/mapper_UxROM.hpp"
#include "nes_emu/mappers/mapper_CNROM.hpp"
#include "nes_emu/mappers/mapper_AxROM.hpp"

namespace NES {

namespace {

typedef std::unique_ptr<Mapper> (*MapperCreator)(Cartridge*);

template <typename MapperType>
std::unique_ptr<Mapper> CreateMapper(Cartridge* cartridge) {
    return std::unique_ptr<Mapper>(new MapperType(cartridge));
}

struct MapperRegistration {
    std::uint16_t id;
    const char* name;
    MapperCreator creator;
};

const MapperRegistration MAPPER_REGISTRY[] = {
    {0, "NROM",  CreateMapper<MapperNROM>},
    {1, "SxROM", CreateMapper<MapperSxROM>},
    {2, "UxROM", CreateMapper<MapperUxROM>},
    {3, "CNROM", CreateMapper<MapperCNROM>},
    {7, "AxROM", CreateMapper<MapperAxROM>},
};

}  // namespace

std::unique_ptr<Mapper> MapperFactory(Cartridge* game) {
    for (const auto& registration : MAPPER_REGISTRY) {
        if (registration.id == game->getMapper())
            return registration.creator(game);
    }
    return nullptr;
}

bool IsMapperSupported(std::uint16_t mapper_id) {
    for (const auto& registration : MAPPER_REGISTRY) {
        if (registration.id == mapper_id)
            return true;
    }
    return false;
}

}  // namespace NES
