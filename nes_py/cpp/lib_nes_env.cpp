#include <stdint.h>
#include <cstring>
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

// definitions of functions for the Python interface to access
extern "C" {
    /// Return the width of the NES.
    EXP unsigned GetNESWidth() { return NESVideoWidth; }

    /// Return the height of the NES.
    EXP unsigned GetNESHeight() { return NESVideoHeight; }

    /// Initialize a new emulator and return a pointer to it
    EXP Emulator* Initialize(wchar_t* path) {
        // convert the c string to a c++ std string data structure
        std::wstring ws_rom_path(path);
        std::string rom_path(ws_rom_path.begin(), ws_rom_path.end());
        // create a new emulator with the given ROM path
        return new Emulator(rom_path);
    }

    /// Return the pointer to the screen buffer
    EXP uint32_t* Screen(Emulator* emu) { return emu->get_screen_buffer(); }

    /// Return the pointer to the memory buffer
    EXP uint8_t* Memory(Emulator* emu) { return emu->get_memory_buffer(); }

    /// Reset the emulator
    EXP void Reset(Emulator* emu) { emu->reset(); }

    /// Perform a discrete step in the emulator (i.e., 1 frame)
    EXP void Step(Emulator* emu, unsigned char action) { emu->step(action); }

    /// Close the emulator, i.e., purge it from memory
    EXP void Close(Emulator* emu) { delete emu; }

    /// Create a deep copy (i.e., a clone) of the given emulator
    EXP Emulator* Clone(Emulator* emu) { return new Emulator(emu); }

}

// un-define the macro
#undef EXP
