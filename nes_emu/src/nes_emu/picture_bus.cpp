//  Program:      nes-py
//  File:         picture_bus.cpp
//  Description:  This class houses picture bus data from the PPU
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "nes_emu/picture_bus.hpp"
#include "nes_emu/log.hpp"

namespace NES {

void PictureBus::refresh_direct_chr_read_pages() {
    direct_chr_read_pages.fill(nullptr);
    if (
        mapper == nullptr ||
        mapper_observes_ppu_reads ||
        mapper_observes_ppu_writes ||
        (
            mapper_observes_ppu_addresses &&
            !mapper->allowsDirectCHRReadWithPPUAddressObservations()
        ) ||
        mapper_has_name_table_mapping
    ) {
        return;
    }

    for (std::size_t page = 0; page < direct_chr_read_pages.size(); ++page) {
        NES_Address page_base = static_cast<NES_Address>(
            page * DIRECT_CHR_READ_PAGE_SIZE
        );
        direct_chr_read_pages[page] = mapper->getDirectCHRReadPage(page_base);
    }
}

NES_Byte PictureBus::read(NES_Address address) {
    address &= 0x3fff;
    if (address < 0x2000) {
        if (mapper_observes_ppu_addresses)
            mapper->onPPUAddress(address);
        const NES_Byte* direct_page = direct_chr_read_pages[
            address / DIRECT_CHR_READ_PAGE_SIZE
        ];
        NES_Byte value = direct_page == nullptr ?
            mapper->readCHR(address) :
            direct_page[address & (DIRECT_CHR_READ_PAGE_SIZE - 1)];
        if (mapper_observes_ppu_reads)
            mapper->onPPURead(address, value);
        return value;
    } else if (address < 0x3f00) {  // Name tables, mirrored through 0x3eff
        NES_Address name_table_address = 0x2000 | (address & 0x0fff);
        if (mapper_observes_ppu_addresses)
            mapper->onPPUAddress(name_table_address);
        if (
            mapper_has_name_table_mapping &&
            mapper->mapsNameTable(name_table_address)
        ) {
            NES_Byte value = mapper->readNameTable(name_table_address);
            if (mapper_observes_ppu_reads)
                mapper->onPPURead(name_table_address, value);
            return value;
        }
        std::size_t table = (name_table_address >> 10) & 0x03;
        NES_Byte value = ram[
            name_tables[table] + (name_table_address & 0x03ff)
        ];
        if (mapper_observes_ppu_reads)
            mapper->onPPURead(name_table_address, value);
        return value;
    } else if (address < 0x4000) {
        return palette[normalize_palette_address(address)];
    }
    return 0;
}

NES_Byte PictureBus::read_sprite_pattern(NES_Address address) {
    if (
        !mapper_requires_ppu_sprite_fetch_address_observations ||
        mapper_observes_ppu_reads ||
        mapper_observes_ppu_writes ||
        mapper_has_name_table_mapping
    ) {
        return read(address);
    }

    address &= 0x1fff;
    const NES_Byte* direct_page = direct_chr_read_pages[
        address / DIRECT_CHR_READ_PAGE_SIZE
    ];
    return direct_page == nullptr ?
        mapper->readCHR(address) :
        direct_page[address & (DIRECT_CHR_READ_PAGE_SIZE - 1)];
}

void PictureBus::write(NES_Address address, NES_Byte value) {
    address &= 0x3fff;
    if (address < 0x2000) {
        if (mapper_observes_ppu_addresses)
            mapper->onPPUAddress(address);
        mapper->writeCHR(address, value);
        if (mapper_observes_ppu_writes)
            mapper->onPPUWrite(address, value);
        ++write_generation;
    } else if (address < 0x3f00) {  // Name tables, mirrored through 0x3eff
        NES_Address name_table_address = 0x2000 | (address & 0x0fff);
        if (mapper_observes_ppu_addresses)
            mapper->onPPUAddress(name_table_address);
        if (
            mapper_has_name_table_mapping &&
            mapper->mapsNameTable(name_table_address)
        ) {
            mapper->writeNameTable(name_table_address, value);
            if (mapper_observes_ppu_writes)
                mapper->onPPUWrite(name_table_address, value);
            ++write_generation;
            return;
        }
        std::size_t table = (name_table_address >> 10) & 0x03;
        ram[name_tables[table] + (name_table_address & 0x03ff)] = value;
        if (mapper_observes_ppu_writes)
            mapper->onPPUWrite(name_table_address, value);
        ++write_generation;
    } else if (address < 0x4000) {
        palette[normalize_palette_address(address)] = value;
    }
}

void PictureBus::update_mirroring() {
    if (mapper == nullptr)
        return;

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
        case FOUR_SCREEN:
            name_tables[0] = 0;
            name_tables[1] = 0x400;
            name_tables[2] = 0x800;
            name_tables[3] = 0xc00;
            LOG(InfoVerbose) <<
                "Four-screen Name Table mirroring set." <<
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
    mapper->clearNameTableMirroringChanged();
}

}  // namespace NES
