//  Program:      nes-py
//  File:         lib_nes_env.cpp
//  Description:  file describes the outward facing ctypes API for Python
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include <exception>
#include <string>
#include "common.hpp"
#include "cartridge.hpp"
#include "emulator.hpp"

// Windows-base systems
#if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__BORLANDC__)
    // setup the module initializer. required to link visual studio C++ ctypes
    void PyInit_lib_nes_env() { }
    // setup the function modifier to export in the DLL
    #define EXP __declspec(dllexport)
// Unix-like systems
#else
    // setup the modifier as a dummy
    #define EXP
#endif

namespace {

std::string ROMPath(wchar_t* path) {
    std::wstring ws_rom_path(path);
    return std::string(ws_rom_path.begin(), ws_rom_path.end());
}

NES::Cartridge LoadCartridgeMetadata(wchar_t* path) {
    NES::Cartridge cartridge;
    cartridge.loadFromFile(ROMPath(path), true);
    return cartridge;
}

thread_local std::string cartridge_error;

}  // namespace

// definitions of functions for the Python interface to access
extern "C" {
    /// Return the width of the NES.
    EXP int Width() {
        return NES::Emulator::WIDTH;
    }

    /// Return the height of the NES.
    EXP int Height() {
        return NES::Emulator::HEIGHT;
    }

    /// Initialize a new emulator and return a pointer to it
    EXP NES::Emulator* Initialize(wchar_t* path) {
        // convert the c string to a c++ std string data structure
        std::string rom_path = ROMPath(path);
        // create a new emulator with the given ROM path
        return new NES::Emulator(rom_path);
    }

    /// Return a native cartridge validation error, or null when valid.
    EXP const char* CartridgeError(wchar_t* path) {
        cartridge_error.clear();
        try {
            NES::Cartridge cartridge;
            cartridge.loadFromFile(ROMPath(path));
            return nullptr;
        } catch (const std::exception& error) {
            cartridge_error = error.what();
        } catch (...) {
            cartridge_error = "unknown cartridge validation error";
        }
        return cartridge_error.c_str();
    }

    /// Return the parsed native mapper number for a ROM path.
    EXP unsigned int CartridgeMapperNumber(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMapper();
    }

    /// Return the parsed native NES 2.0 submapper number for a ROM path.
    EXP unsigned int CartridgeSubmapper(wchar_t* path) {
        return LoadCartridgeMetadata(path).getSubmapper();
    }

    /// Return the parsed native PRG ROM size in bytes.
    EXP std::size_t CartridgePRGROMSize(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().prg_rom_size;
    }

    /// Return the parsed native PRG ROM bank count.
    EXP std::size_t CartridgePRGROMBanks(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().prg_rom_banks;
    }

    /// Return the parsed native CHR ROM size in bytes.
    EXP std::size_t CartridgeCHRROMSize(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().chr_rom_size;
    }

    /// Return the parsed native CHR ROM bank count.
    EXP std::size_t CartridgeCHRROMBanks(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().chr_rom_banks;
    }

    /// Return the parsed native ordinary PRG RAM size in bytes.
    EXP std::size_t CartridgePRGRAMSize(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().prg_ram_size;
    }

    /// Return the parsed native battery-backed PRG RAM size in bytes.
    EXP std::size_t CartridgePRGBatteryRAMSize(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().prg_battery_ram_size;
    }

    /// Return the parsed native ordinary CHR RAM size in bytes.
    EXP std::size_t CartridgeCHRRAMSize(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().chr_ram_size;
    }

    /// Return the parsed native battery-backed CHR RAM size in bytes.
    EXP std::size_t CartridgeCHRBatteryRAMSize(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().chr_battery_ram_size;
    }

    /// Return the parsed native trainer flag.
    EXP int CartridgeHasTrainer(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().has_trainer ? 1 : 0;
    }

    /// Return the parsed native trainer start offset.
    EXP std::size_t CartridgeTrainerStart(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().trainer_start;
    }

    /// Return the parsed native trainer stop offset.
    EXP std::size_t CartridgeTrainerStop(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().trainer_stop;
    }

    /// Return the parsed native battery flag.
    EXP int CartridgeHasBattery(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().has_battery ? 1 : 0;
    }

    /// Return the parsed native header mirroring mode.
    EXP unsigned int CartridgeNameTableMirroring(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().name_table_mirroring;
    }

    /// Return the parsed native VS Unisystem flag.
    EXP int CartridgeHasVSUnisystem(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().has_vs_unisystem ? 1 : 0;
    }

    /// Return the parsed native PlayChoice-10 flag.
    EXP int CartridgeHasPlayChoice10(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().has_play_choice_10 ? 1 : 0;
    }

    /// Return the parsed native PAL flag.
    EXP int CartridgeIsPAL(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().is_pal ? 1 : 0;
    }

    /// Return the parsed native NES 2.0 header flag.
    EXP int CartridgeIsNES2(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().is_nes2 ? 1 : 0;
    }

    /// Return a pointer to a controller on the machine
    EXP NES::NES_Byte* Controller(NES::Emulator* emu, int port) {
        return emu->get_controller(port);
    }

    /// Return the pointer to the screen buffer
    EXP NES::NES_Pixel* Screen(NES::Emulator* emu) {
        return emu->get_screen_buffer();
    }

    /// Return the pointer to the memory buffer
    EXP NES::NES_Byte* Memory(NES::Emulator* emu) {
        return emu->get_memory_buffer();
    }

    /// Return the active mapper number.
    EXP int MapperNumber(NES::Emulator* emu) {
        return emu->get_mapper_number();
    }

    /// Return the PRG ROM size in bytes.
    EXP unsigned int PRGROMSize(NES::Emulator* emu) {
        return emu->get_prg_rom_size();
    }

    /// Return the CHR ROM size in bytes.
    EXP unsigned int CHRROMSize(NES::Emulator* emu) {
        return emu->get_chr_rom_size();
    }

    /// Return 1 when the active mapper uses CHR RAM, 0 for CHR ROM.
    EXP int HasCHRRAM(NES::Emulator* emu) {
        return emu->has_chr_ram() ? 1 : 0;
    }

    /// Return the active mapper mirroring mode.
    EXP int NameTableMirroring(NES::Emulator* emu) {
        return emu->get_name_table_mirroring();
    }

    /// Read active mapper PRG memory.
    EXP unsigned int ReadPRG(NES::Emulator* emu, unsigned int address) {
        return emu->read_prg(static_cast<NES::NES_Address>(address));
    }

    /// Write active mapper PRG memory.
    EXP void WritePRG(NES::Emulator* emu, unsigned int address, unsigned int value) {
        emu->write_prg(
            static_cast<NES::NES_Address>(address),
            static_cast<NES::NES_Byte>(value)
        );
    }

    /// Read active mapper CHR memory.
    EXP unsigned int ReadCHR(NES::Emulator* emu, unsigned int address) {
        return emu->read_chr(static_cast<NES::NES_Address>(address));
    }

    /// Write active mapper CHR memory.
    EXP void WriteCHR(NES::Emulator* emu, unsigned int address, unsigned int value) {
        emu->write_chr(
            static_cast<NES::NES_Address>(address),
            static_cast<NES::NES_Byte>(value)
        );
    }

    /// Reset the emulator
    EXP void Reset(NES::Emulator* emu) {
        emu->reset();
    }

    /// Perform a discrete step in the emulator (i.e., 1 frame)
    EXP void Step(NES::Emulator* emu) {
        emu->step();
    }

    /// Create a deep copy (i.e., a clone) of the given emulator
    EXP void Backup(NES::Emulator* emu) {
        emu->backup();
    }

    /// Create a deep copy (i.e., a clone) of the given emulator
    EXP void Restore(NES::Emulator* emu) {
        emu->restore();
    }

    /// Close the emulator, i.e., purge it from memory
    EXP void Close(NES::Emulator* emu) {
        delete emu;
    }
}

// un-define the macro
#undef EXP
