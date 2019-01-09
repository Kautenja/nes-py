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
        return ram[address & 0x7ff];
    else if (address < 0x4020) {
        // PPU registers, mirrored
        if (address < 0x4000) {
            auto it = read_callbacks.find(static_cast<IORegisters>(address & 0x2007));
            if (it != read_callbacks.end())
                return (it -> second)();
                // Second object is the pointer to the function object
                // Dereference the function pointer and call it
            else
                LOG(InfoVerbose) << "No read callback registered for I/O register at: " << std::hex << +address << std::endl;
        }
        // only *some* IO registers
        else if (address < 0x4018 && address >= 0x4014) {
            auto it = read_callbacks.find(static_cast<IORegisters>(address));
            if (it != read_callbacks.end())
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
        if (mapper->hasExtendedRAM()) {
            return extended_ram[address - 0x6000];
        }
    }
    else {
        return mapper->readPRG(address);
    }
    return 0;
}

void MainBus::write(NES_Address address, NES_Byte value) {
    if (address < 0x2000)
        ram[address & 0x7ff] = value;
    else if (address < 0x4020) {
        //PPU registers, mirrored
        if (address < 0x4000) {
            auto it = write_callbacks.find(static_cast<IORegisters>(address & 0x2007));
            if (it != write_callbacks.end())
                (it -> second)(value);
                //Second object is the pointer to the function object
                //Dereference the function pointer and call it
            else
                LOG(InfoVerbose) << "No write callback registered for I/O register at: " << std::hex << +address << std::endl;
        }
        //only some registers
        else if (address < 0x4017 && address >= 0x4014) {
            auto it = write_callbacks.find(static_cast<IORegisters>(address));
            if (it != write_callbacks.end())
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
        if (mapper->hasExtendedRAM()) {
            extended_ram[address - 0x6000] = value;
        }
    }
    else {
        mapper->writePRG(address, value);
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
    else if (address < 0x8000)
        if (mapper->hasExtendedRAM())
            return &extended_ram[address - 0x6000];

    return nullptr;
}

void MainBus::set_mapper(Mapper* mapper) {
    this->mapper = mapper;
    if (mapper->hasExtendedRAM())
        extended_ram.resize(0x2000);
}
