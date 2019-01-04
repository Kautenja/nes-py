/// File: python_api.py
/// Description: The API definition for ctypes in Python.
///
#include <cstdint>
#include <cstring>
#include "emulator.hpp"

// Windows-base systems
#if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__BORLANDC__)
    // setup the module initializer. required to link visual studio C++ ctypes
    void PyInit_lib_nes_env() { }
    // setup the function modifier to export in the DLL
    #define external __declspec(dllexport)
// Unix-like systems
#else
    // setup the modifier as a dummy
    #define external
#endif

// definitions of functions for the Python interface to access
extern "C" {
    /// Initialize a new emulator and return a pointer to it
    external Emulator* Initialize(wchar_t* path) {
        // convert the c string to a c++ std string data structure
        std::wstring ws_rom_path(path);
        std::string rom_path(ws_rom_path.begin(), ws_rom_path.end());
        // create a new emulator with the given ROM path
        return new Emulator(rom_path);
    }

    /// Return the width of the NES.
    external unsigned GetNESWidth() {
        return NESVideoWidth;
    }

    /// Return the height of the NES.
    external unsigned GetNESHeight() {
        return NESVideoHeight;
    }

    /// Read a byte from memory
    external uint8_t ReadMemory(Emulator* emulator, uint16_t address) {
        return emulator->read_memory(address);
    }

    /// Set a byte in memory
    external void WriteMemory(Emulator* emulator, uint16_t address, uint8_t value) {
        emulator->write_memory(address, value);
    }

    /// Return the pointer to the screen buffer
    external uint32_t* GetScreenBuffer(Emulator* emulator) {
        return emulator->get_screen_buffer();
    }

    /// Reset the emulator
    external void Reset(Emulator* emulator) {
        emulator->reset();
    }

    /// Perform a discrete step in the emulator (i.e., 1 frame)
    external void Step(Emulator* emulator, unsigned char action) {
        emulator->step(action);
    }

    /// Close the emulator, i.e., purge it from memory
    external void Close(Emulator* emulator) {
        delete emulator;
    }

    /// Backup the game state in the emulator
    external void Backup(Emulator* emulator) {
        emulator->backup();
    }

    /// Restore a game state in the emulator
    external void Restore(Emulator* emulator) {
        emulator->restore();
    }

}
