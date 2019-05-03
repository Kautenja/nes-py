//  Program:      nes-py
//  File:         common.hpp
//  Description:  This file defines common types used in the project
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef COMMON_HPP
#define COMMON_HPP

// resolve an issue with MSVC overflow during compilation (Windows)
#define _CRT_DECLARE_NONSTDC_NAMES 0
#include <stdint.h>

/// A shortcut for a byte
using NES_Byte = uint8_t;
/// A shortcut for a memory address (16-bit)
using NES_Address = uint16_t;
/// A shortcut for a single pixel in memory
using NES_Pixel = uint32_t;

#endif // COMMON_HPP
