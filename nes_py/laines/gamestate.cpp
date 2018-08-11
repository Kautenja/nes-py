#include "gamestate.hpp"

GameState::GameState() {

}

GameState::GameState(GameState* state) {
    // cartridge = new Cartridge(state->cartridge);
    // joypad = new Joypad(state->joypad);
    // gui = new GUI(state->gui);
}

void GameState::load() {
    // load the cart
    CPU::set_cartridge(cartridge);
    PPU::set_cartridge(cartridge);
    // set the joypad
    CPU::set_joypad(joypad);
    // set the gui
    PPU::set_gui(gui);
}
