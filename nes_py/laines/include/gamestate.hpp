#include "cartridge.hpp"
#include "gui.hpp"
#include "joypad.hpp"
#include "cpu.hpp"
#include "ppu.hpp"

/// a save state of the NES machine
class GameState {
private:
    /// the state for the CPU
    PPUState* ppu_state;
    /// the state for the GPU
    CPUState* cpu_state;

public:
    /// the current game cartridge
    Cartridge* cartridge;
    /// the joy-pad for the game-state
    Joypad* joypad;
    /// the GUI for the game-state
    GUI* gui;

    /// Initialize a new game-state
    GameState();
    /// Delete a game-state
    ~GameState();
    /// create a new game-state as a copy of another
    GameState(GameState* state);
    /// Set the PPU state to a new value
    void set_ppu_state(PPUState* new_ppu_state);
    /// Set the CPU state to a new value
    void set_cpu_state(CPUState* new_cpu_state);
    /// Load the game-state's data into the machine
    void load();
};
