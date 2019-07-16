//  Program:      nes-py
//  File:         picture_bus.cpp
//  Description:  This class houses picture bus data from the PPU
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "picture_bus.hpp"
#include "log.hpp"

namespace NES {

NES_Byte PictureBus::read(NES_Address address) {
    if (address < 0x2000) {
        return mapper->readCHR(address);
    } else if (address < 0x3eff) {  // Name tables up to 0x3000, then mirrored up to 0x3ff
        if (address < 0x2400)  // NT0
            return ram[name_tables[0] + (address & 0x3ff)];
        else if (address < 0x2800)  // NT1
            return ram[name_tables[1] + (address & 0x3ff)];
        else if (address < 0x2c00)  // NT2
            return ram[name_tables[2] + (address & 0x3ff)];
        else  // NT3
            return ram[name_tables[3] + (address & 0x3ff)];
    } else if (address < 0x3fff) {
        return palette[address & 0x1f];
    }
    return 0;
}

void PictureBus::write(NES_Address address, NES_Byte value) {
    if (address < 0x2000) {
        mapper->writeCHR(address, value);
    } else if (address < 0x3eff) {  // Name tables up to 0x3000, then mirrored up to 0x3ff
        if (address < 0x2400)  // NT0
            ram[name_tables[0] + (address & 0x3ff)] = value;
        else if (address < 0x2800)  // NT1
            ram[name_tables[1] + (address & 0x3ff)] = value;
        else if (address < 0x2c00)  // NT2
            ram[name_tables[2] + (address & 0x3ff)] = value;
        else  // NT3
            ram[name_tables[3] + (address & 0x3ff)] = value;
    } else if (address < 0x3fff) {
        if (address == 0x3f10)
            palette[0] = value;
        else
            palette[address & 0x1f] = value;
    }
}

void PictureBus::update_mirroring() {
    switch (mapper->getNameTableMirroring()) {
        case HORIZONTAL:
            name_tables[0] = name_tables[1] = 0;
            name_tables[2] = name_tables[3] = 0x400;
            LOG(InfoVerbose) <<
                "Horizontal Name Table mirroring set. (Vertical Scrolling)" <<
                std::endl;
            break;
        case VERTICAL:
            name_tables[0] = name_tables[2] = 0;
            name_tables[1] = name_tables[3] = 0x400;
            LOG(InfoVerbose) <<
                "Vertical Name Table mirroring set. (Horizontal Scrolling)" <<
                std::endl;
            break;
        case ONE_SCREEN_LOWER:
            name_tables[0] = name_tables[1] = name_tables[2] = name_tables[3] = 0;
            LOG(InfoVerbose) <<
                "Single Screen mirroring set with lower bank." <<
                std::endl;
            break;
        case ONE_SCREEN_HIGHER:
            name_tables[0] = name_tables[1] = name_tables[2] = name_tables[3] = 0x400;
            LOG(InfoVerbose) <<
                "Single Screen mirroring set with higher bank." <<
                std::endl;
            break;
        default:
            name_tables[0] = name_tables[1] = name_tables[2] = name_tables[3] = 0;
            LOG(Error) <<
                "Unsupported Name Table mirroring : " <<
                mapper->getNameTableMirroring() <<
                std::endl;
    }
}

}  // namespace NES
