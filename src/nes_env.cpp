// TODO define header file and cleanup this massive file
// TODO consider defining the extern "C" in a separate file?
// TODO organize imports

// #include <iostream>
#include <cstdint>
#include <string>

#include <SDL2/SDL.h>
// #include <SDL2/SDL_image.h>

#include "cartridge.hpp"



/// An OpenAI Gym like interface to the LaiNES emulator.
class NESEnv{

private:
    /// the path to the ROM for the emulator to run
    std::string path;

    /// the window to send frames to
    SDL_Window* window;
    /// the renderer for generating frames
    SDL_Renderer* renderer;
    /// the texture (surface) to render frames on
    SDL_Texture* gameTexture;

    /// Setup the graphics components for the emulator
    void setupSDL() {
        // Initialize graphics system:
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

        // Initialize the window to render to
        this->window = SDL_CreateWindow(
            "laines-py",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            this->WIDTH,
            this->HEIGHT,
            0
        );

        // initialize the renderer for sending frames to the window
        this->renderer = SDL_CreateRenderer(
            this->window,
            -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
        );
        SDL_RenderSetLogicalSize(renderer, WIDTH, HEIGHT);

        // initialize the surface for game frames to blit to
        this->gameTexture = SDL_CreateTexture(
            this->renderer,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            this->WIDTH,
            this->HEIGHT
        );
    }

public:

    /// the width of the screen in pixels
    const unsigned WIDTH  = 256;
    /// the height of the screen in pixels
    const unsigned HEIGHT = 240;

    /**
        Initialize a new NESEnv.

        @param path the path to the ROM for the emulator to load
        @returns a new instance of NESEnv for a given ROM

    */
    NESEnv(wchar_t* path) {
        // convert the wchar_t type to a string
        std::wstring ws(path);
        std::string str(ws.begin(), ws.end());
        this->path = str;
        this->setupSDL();
    }

    /// Reset the emulator to its initial state.
    void reset() {
        // load the cartridge based on the environment's ROM path
        Cartridge::load(this->path.c_str());
    }

    /**
        Perform a discrete "step" of the NES by rendering 1 frame.

        @param action the controller bitmap of which buttons to press.
        The parameter uses 1 for "pressed" and 0 for "not pressed".
        It uses the following mapping of bits to buttons:
        7: UP
        6: LEFT
        5: DOWN
        4: RIGHT
        3: SELECT
        2: START
        1: B
        0: A

    */
    void step(uint8_t action) {

    }

    /**
        Return the screen of the emulator.

        @returns TODO

    */
    void get_screen() {

    }

};


// definitions of functions for the Python interface to access
extern "C" {
    /// the initializer to return a new NESEnv with a given path
    NESEnv* NESEnv_init(wchar_t* path){ return new NESEnv(path); }

    /// the function to reset the environment
    void NESEnv_reset(NESEnv* env) { env->reset(); }

    /// the function to destroy an NESEnv and clear it from memory
    void NESEnv_close(NESEnv* env) { delete env; }
}
