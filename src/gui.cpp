#include <csignal>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "Sound_Queue.h"
#include "cartridge.hpp"
#include "cpu.hpp"
#include "gui.hpp"
#include "config.hpp"

namespace GUI {

// Screen size:
const unsigned WIDTH  = 256;
const unsigned HEIGHT = 240;

// SDL structures:
SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* gameTexture;
SDL_Texture* background;
u8 const* keys;
SDL_Joystick* joystick[] = { nullptr, nullptr };


/* Initialize GUI */
void init(std::string path) {
    // Initialize graphics system:
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    for (int i = 0; i < SDL_NumJoysticks(); i++)
        joystick[i] = SDL_JoystickOpen(i);

    // Initialize graphics structures:
    window = SDL_CreateWindow("laines-py",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              WIDTH * last_window_size, HEIGHT * last_window_size, 0);

    renderer = SDL_CreateRenderer(window, -1,
                                  SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_RenderSetLogicalSize(renderer, WIDTH, HEIGHT);

    gameTexture = SDL_CreateTexture(renderer,
                                    SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                    WIDTH, HEIGHT);

    keys = SDL_GetKeyboardState(0);

    // load the cartridge based on the given path
    Cartridge::load(path.c_str());
}

/* Get the joypad state from SDL */
u8 get_joypad_state(int n) {
    const int DEAD_ZONE = 8000;

    u8 j = 0;
    if (useJoystick[n]) {
        j |= (SDL_JoystickGetButton(joystick[n], BTN_A[n]))      << 0;  // A.
        j |= (SDL_JoystickGetButton(joystick[n], BTN_B[n]))      << 1;  // B.
        j |= (SDL_JoystickGetButton(joystick[n], BTN_SELECT[n])) << 2;  // Select.
        j |= (SDL_JoystickGetButton(joystick[n], BTN_START[n]))  << 3;  // Start.

        j |= (SDL_JoystickGetButton(joystick[n], BTN_UP[n]))     << 4;  // Up.
        j |= (SDL_JoystickGetAxis(joystick[n], 1) < -DEAD_ZONE)  << 4;
        j |= (SDL_JoystickGetButton(joystick[n], BTN_DOWN[n]))   << 5;  // Down.
        j |= (SDL_JoystickGetAxis(joystick[n], 1) >  DEAD_ZONE)  << 5;
        j |= (SDL_JoystickGetButton(joystick[n], BTN_LEFT[n]))   << 6;  // Left.
        j |= (SDL_JoystickGetAxis(joystick[n], 0) < -DEAD_ZONE)  << 6;
        j |= (SDL_JoystickGetButton(joystick[n], BTN_RIGHT[n]))  << 7;  // Right.
        j |= (SDL_JoystickGetAxis(joystick[n], 0) >  DEAD_ZONE)  << 7;
    }
    else {
        j |= (keys[KEY_A[n]])      << 0;
        j |= (keys[KEY_B[n]])      << 1;
        j |= (keys[KEY_SELECT[n]]) << 2;
        j |= (keys[KEY_START[n]])  << 3;
        j |= (keys[KEY_UP[n]])     << 4;
        j |= (keys[KEY_DOWN[n]])   << 5;
        j |= (keys[KEY_LEFT[n]])   << 6;
        j |= (keys[KEY_RIGHT[n]])  << 7;
    }
    return j;
}

/* Send the rendered frame to the GUI */
void new_frame(u32* pixels) {
    SDL_UpdateTexture(gameTexture, NULL, pixels, WIDTH * sizeof(u32));
}

/* Render the screen */
void render() {
    SDL_RenderClear(renderer);

    // Draw the NES screen:
    if (Cartridge::loaded())
        SDL_RenderCopy(renderer, gameTexture, NULL, NULL);
    else
        SDL_RenderCopy(renderer, background, NULL, NULL);

    SDL_RenderPresent(renderer);
}

/* Run the emulator */
void run() {
    SDL_Event e;

    while (true) {
        // Handle events
        while (SDL_PollEvent(&e))
            switch (e.type) {
                case SDL_QUIT: return;
                case SDL_KEYDOWN: if (keys[SDL_SCANCODE_ESCAPE]) return;
            }
        // run a frame and render it
        CPU::run_frame();
        render();
    }
}


}
