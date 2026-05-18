//  Program:      nes-py
//  File:         main_bus.cpp
//  Description:  This class houses the main bus data for the NES
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "nes_emu/main_bus.hpp"
#include "nes_emu/controller.hpp"
#include "nes_emu/cpu.hpp"
#include "nes_emu/log.hpp"
#include "nes_emu/picture_bus.hpp"
#include "nes_emu/ppu.hpp"

namespace NES {

void MainBus::refresh_direct_prg_read_pages() {
    direct_prg_read_pages.fill(nullptr);
    if (mapper == nullptr)
        return;

    for (std::size_t page = 0; page < direct_prg_read_pages.size(); ++page) {
        NES_Address page_base = static_cast<NES_Address>(
            0x8000 + page * DIRECT_PRG_READ_PAGE_SIZE
        );
        direct_prg_read_pages[page] = mapper->getDirectPRGReadPage(page_base);
    }
}

NES_Byte MainBus::read(NES_Address address) {
    if (address < 0x2000) {
        return ram[address & 0x7ff];
    } else if (address < 0x4020) {
        if (address < 0x4000) {  // PPU registers, mirrored
            if (ppu != nullptr) {
                switch (address & 0x2007) {
                    case PPUSTATUS:
                        return ppu->get_status();
                    case OAMDATA:
                        return ppu->get_OAM_data();
                    case PPUDATA:
                        if (picture_bus != nullptr)
                            return ppu->get_data(*picture_bus);
                        break;
                    default:
                        break;
                }
            }
            const auto& callback = ppu_read_callbacks[address & 0x0007];
            if (callback)
                return callback();
            else
                LOG(InfoVerbose) << "No read callback registered for I/O register at: " << std::hex << +address << std::endl;
        } else if (address < 0x4018 && address >= 0x4014) {  // only *some* IO registers
            if (controllers != nullptr) {
                switch (address) {
                    case JOY1:
                        return controllers[0].read();
                    case JOY2:
                        return controllers[1].read();
                    default:
                        break;
                }
            }
            const auto& callback = io_read_callbacks[address - 0x4014];
            if (callback)
                return callback();
            else
                LOG(InfoVerbose) << "No read callback registered for I/O register at: " << std::hex << +address << std::endl;
        }
        else {
            LOG(InfoVerbose) << "Read access attempt at: " << std::hex << +address << std::endl;
        }
    } else if (address < 0x6000) {
        if (mapper != nullptr && mapper->handlesExpansion(address))
            return mapper->readExpansion(address);
        LOG(InfoVerbose) << "Expansion ROM read attempted. This is currently unsupported" << std::endl;
    } else if (address < 0x8000) {
        if (mapper != nullptr)
            return mapper->readPRGRAM(address);
    } else {
        const NES_Byte* direct_page = direct_prg_read_pages[
            (address - 0x8000) / DIRECT_PRG_READ_PAGE_SIZE
        ];
        if (direct_page != nullptr)
            return direct_page[address & (DIRECT_PRG_READ_PAGE_SIZE - 1)];
        return mapper->readPRG(address);
    }
    return 0;
}

void MainBus::write(NES_Address address, NES_Byte value) {
    if (address < 0x2000) {
        ram[address & 0x7ff] = value;
    } else if (address < 0x4020) {
        if (address < 0x4000) {  // PPU registers, mirrored
            if (ppu != nullptr) {
                switch (address & 0x2007) {
                    case PPUCTRL:
                        ppu->control(value);
                        return;
                    case PPUMASK:
                        ppu->set_mask(value);
                        return;
                    case OAMADDR:
                        ppu->set_OAM_address(value);
                        return;
                    case OAMDATA:
                        ppu->set_OAM_data(value);
                        return;
                    case PPUSCROL:
                        ppu->set_scroll(value);
                        return;
                    case PPUADDR:
                        ppu->set_data_address(value);
                        return;
                    case PPUDATA:
                        if (picture_bus != nullptr) {
                            ppu->set_data(*picture_bus, value);
                            return;
                        }
                        break;
                    default:
                        break;
                }
            }
            const auto& callback = ppu_write_callbacks[address & 0x0007];
            if (callback)
                return callback(value);
            else
                LOG(InfoVerbose) << "No write callback registered for I/O register at: " << std::hex << +address << std::endl;
        } else if (address < 0x4017 && address >= 0x4014) {  // only some registers
            if (address == OAMDMA && cpu != nullptr && ppu != nullptr) {
                cpu->skip_DMA_cycles();
                ppu->do_DMA(get_page_pointer(value));
                return;
            }
            if (address == JOY1 && controllers != nullptr) {
                controllers[0].strobe(value);
                controllers[1].strobe(value);
                return;
            }
            const auto& callback = io_write_callbacks[address - 0x4014];
            if (callback)
                return callback(value);
            else
                LOG(InfoVerbose) << "No write callback registered for I/O register at: " << std::hex << +address << std::endl;
        } else {
            LOG(InfoVerbose) << "Write access attmept at: " << std::hex << +address << std::endl;
        }
    } else if (address < 0x6000) {
        if (mapper != nullptr && mapper->handlesExpansion(address)) {
            mapper->writeExpansion(address, value);
            return;
        }
        LOG(InfoVerbose) << "Expansion ROM access attempted. This is currently unsupported" << std::endl;
    } else if (address < 0x8000) {
        if (mapper != nullptr)
            mapper->writePRGRAM(address, value);
    } else {
        mapper->writePRG(address, mapper->resolveBusConflict(address, value));
        refresh_direct_prg_read_pages();
        if (picture_bus != nullptr)
            picture_bus->refresh_direct_chr_read_pages();
    }
}

const NES_Byte* MainBus::get_page_pointer(NES_Byte page) {
    NES_Address address = page << 8;
    if (address < 0x2000)
        return &ram[address & 0x7ff];
    else if (address < 0x4020)
        LOG(Error) << "Register address memory pointer access attempt" << std::endl;
    else if (address < 0x6000)
        LOG(Error) << "Expansion ROM access attempted, which is unsupported" << std::endl;
    else if (address < 0x8000) {
        if (mapper != nullptr)
            return mapper->getPRGRAMPointer(address);
    }

    return nullptr;
}

}  // namespace NES
