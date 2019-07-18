//  Program:      nes-py
//  File:         mapper_CNROM.cpp
//  Description:  An implementation of the CNROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "mappers/mapper_CNROM.hpp"
#include "log.hpp"

namespace NES {

void MapperCNROM::writeCHR(NES_Address address, NES_Byte value) {
    LOG(Info) <<
        "Read-only CHR memory write attempt at " <<
        std::hex <<
        address <<
        std::endl;
}

}  // namespace NES
