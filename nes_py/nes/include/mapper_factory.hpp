//  Program:      nes-py
//  File:         mapper.hpp
//  Description:  An abstract factory for mappers
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef MAPPER_FACTORY_HPP
#define MAPPER_FACTORY_HPP

#include <cstdint>
#include <memory>
#include "mapper.hpp"

namespace NES {

/// Create a mapper for the given cartridge.
///
/// @param game the cartridge to initialize a mapper for
///
std::unique_ptr<Mapper> MapperFactory(Cartridge* game);

/// Return true when a mapper ID has a registered implementation.
bool IsMapperSupported(std::uint16_t mapper_id);

}  // namespace NES

#endif  // MAPPER_FACTORY_HPP
