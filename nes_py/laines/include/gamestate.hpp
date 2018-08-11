#include "cartridge.hpp"
#include "gui.hpp"
#include "joypad.hpp"
#include "cpu.hpp"
#include "ppu.hpp"

/// a saveable state of the NES machine
class GameState {
public:
    /// the current game cartridge
    Cartridge* cartridge;
    /// the joypad for the gamestate
    Joypad* joypad;
    /// the GUI for the gamestate
    GUI* gui;
    /// the state for the CPU
    PPUState ppu;
    /// the state for the GPU
    CPUState cpu;

    /// Initialize a new gamestate
    GameState();
    /// create a new gamestate as a copy of another
    GameState(GameState* state);
    /// Load the gamestate's data into the machine
    void load();
};
