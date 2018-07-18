#include <csignal>
#include <SDL2/SDL_image.h>
#include "cartridge.hpp"
#include "cpu.hpp"
#include "gui.hpp"
#include "config.hpp"

namespace GUI {

    // Screen size
    const unsigned WIDTH  = 256;
    const unsigned HEIGHT = 240;

    // SDL structures
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* gameTexture;
    u8 const* keys;
    SDL_Joystick* joystick[] = { nullptr, nullptr };


    /* Initialize GUI */
    void init() {
        // Initialize graphics system:
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

        // TODO: destroy
        for (int i = 0; i < SDL_NumJoysticks(); i++)
            joystick[i] = SDL_JoystickOpen(i);

        // Initialize graphics structures:
        window = SDL_CreateWindow(
            "laines-py",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            WIDTH,
            HEIGHT,
            0
        );

        renderer = SDL_CreateRenderer(
            window,
            -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
        );
        SDL_RenderSetLogicalSize(renderer, WIDTH, HEIGHT);

        gameTexture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            WIDTH,
            HEIGHT
        );

        // TODO: destroy
        keys = SDL_GetKeyboardState(0);
    }

    /* Send the rendered frame to the GUI */
    void new_frame(u32* pixels) {
        SDL_UpdateTexture(gameTexture, NULL, pixels, WIDTH * sizeof(u32));
    }

    /* Render the screen */
    void render() {
        // Clear the screen
        SDL_RenderClear(renderer);
        // Draw the NES screen:
        SDL_RenderCopy(renderer, gameTexture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

}
