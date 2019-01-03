#include "gamestate.hpp"

GameState::GameState() {
    ppu_state = new PPUState();
    cpu_state = new CPUState();
};

GameState::~GameState() {
    delete cartridge;
    delete joypad;
    delete gui;
    delete ppu_state;
    delete cpu_state;
}

GameState::GameState(GameState* state) {
    cartridge = new Cartridge(state->cartridge);
    joypad = new Joypad(state->joypad);
    gui = new GUI(state->gui);
    cpu_state = new CPUState(state->cpu_state);
    ppu_state = new PPUState(state->ppu_state);
}

GameState::GameState(GameState* state, CPUState* cpu, PPUState* ppu) {
    cartridge = new Cartridge(state->cartridge);
    joypad = new Joypad(state->joypad);
    gui = new GUI(state->gui);
    cpu_state = cpu;
    ppu_state = ppu;
}

void GameState::load() {
    // setup the CPU up
    CPU::set_state(cpu_state);
    CPU::set_cartridge(cartridge);
    CPU::set_joypad(joypad);
    // set the PPU up
    PPU::set_state(ppu_state);
    PPU::set_gui(gui);
    PPU::set_cartridge(cartridge);
}
