//  Program:      nes-py
//  File:         main_bus.cpp
//  Description:  This class houses the main bus data for the NES
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "main_bus.hpp"
#include "log.hpp"

NES_Byte MainBus::read(NES_Address address) {
    if (address < 0x2000)
        return m_RAM[address & 0x7ff];
    else if (address < 0x4020) {
        // PPU registers, mirrored
        if (address < 0x4000) {
            auto it = m_readCallbacks.find(static_cast<IORegisters>(address & 0x2007));
            if (it != m_readCallbacks.end())
                return (it -> second)();
                // Second object is the pointer to the function object
                // Dereference the function pointer and call it
            else
                LOG(InfoVerbose) << "No read callback registered for I/O register at: " << std::hex << +address << std::endl;
        }
        // only *some* IO registers
        else if (address < 0x4018 && address >= 0x4014) {
            auto it = m_readCallbacks.find(static_cast<IORegisters>(address));
            if (it != m_readCallbacks.end())
                return (it -> second)();
                // Second object is the pointer to the function object
                // Dereference the function pointer and call it
            else
                LOG(InfoVerbose) << "No read callback registered for I/O register at: " << std::hex << +address << std::endl;
        }
        else
            LOG(InfoVerbose) << "Read access attempt at: " << std::hex << +address << std::endl;
    }
    else if (address < 0x6000) {
        LOG(InfoVerbose) << "Expansion ROM read attempted. This is currently unsupported" << std::endl;
    }
    else if (address < 0x8000) {
        if (m_mapper->hasExtendedRAM()) {
            return m_extRAM[address - 0x6000];
        }
    }
    else {
        return m_mapper->readPRG(address);
    }
    return 0;
}

void MainBus::write(NES_Address address, NES_Byte value) {
    if (address < 0x2000)
        m_RAM[address & 0x7ff] = value;
    else if (address < 0x4020) {
        //PPU registers, mirrored
        if (address < 0x4000) {
            auto it = m_writeCallbacks.find(static_cast<IORegisters>(address & 0x2007));
            if (it != m_writeCallbacks.end())
                (it -> second)(value);
                //Second object is the pointer to the function object
                //Dereference the function pointer and call it
            else
                LOG(InfoVerbose) << "No write callback registered for I/O register at: " << std::hex << +address << std::endl;
        }
        //only some registers
        else if (address < 0x4017 && address >= 0x4014) {
            auto it = m_writeCallbacks.find(static_cast<IORegisters>(address));
            if (it != m_writeCallbacks.end())
                (it -> second)(value);
                //Second object is the pointer to the function object
                //Dereference the function pointer and call it
            else
                LOG(InfoVerbose) << "No write callback registered for I/O register at: " << std::hex << +address << std::endl;
        }
        else
            LOG(InfoVerbose) << "Write access attmept at: " << std::hex << +address << std::endl;
    }
    else if (address < 0x6000) {
        LOG(InfoVerbose) << "Expansion ROM access attempted. This is currently unsupported" << std::endl;
    }
    else if (address < 0x8000) {
        if (m_mapper->hasExtendedRAM()) {
            m_extRAM[address - 0x6000] = value;
        }
    }
    else {
        m_mapper->writePRG(address, value);
    }
}

const NES_Byte* MainBus::get_page_pointer(NES_Byte page) {
    NES_Address address = page << 8;
    if (address < 0x2000)
        return &m_RAM[address & 0x7ff];
    else if (address < 0x4020)
        LOG(Error) << "Register address memory pointer access attempt" << std::endl;
    else if (address < 0x6000)
        LOG(Error) << "Expansion ROM access attempted, which is unsupported" << std::endl;
    else if (address < 0x8000)
        if (m_mapper->hasExtendedRAM())
            return &m_extRAM[address - 0x6000];

    return nullptr;
}

bool MainBus::set_mapper(Mapper* mapper) {
    m_mapper = mapper;

    if (!mapper) {
        LOG(Error) << "Mapper pointer is nullptr" << std::endl;
        return false;
    }

    if (mapper->hasExtendedRAM())
        m_extRAM.resize(0x2000);

    return true;
}

bool MainBus::set_write_callback(IORegisters reg, std::function<void(NES_Byte)> callback) {
    if (!callback) {
        LOG(Error) << "callback argument is nullptr" << std::endl;
        return false;
    }
    return m_writeCallbacks.emplace(reg, callback).second;
}

bool MainBus::set_read_callback(IORegisters reg, std::function<NES_Byte(void)> callback) {
    if (!callback) {
        LOG(Error) << "callback argument is nullptr" << std::endl;
        return false;
    }
    return m_readCallbacks.emplace(reg, callback).second;
}
