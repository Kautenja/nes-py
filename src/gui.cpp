#include <csignal>
#include <SDL2/SDL_image.h>
#include "cartridge.hpp"
#include "cpu.hpp"
#include "gui.hpp"

/// The graphics interface to SDL
namespace GUI {

    /// the width of the screen in pixels
    const unsigned WIDTH  = 256;
    /// Return the width of the screen.
    unsigned get_width() { return WIDTH; }
    /// the height of the screen in pixels
    const unsigned HEIGHT = 240;
    /// Return the height of the screen.
    unsigned get_height() { return HEIGHT; }

    /// the SDL window to send surfaces to
    SDL_Window* window;
    /// the render for drawing on surfaces
    SDL_Renderer* renderer;
    /// the text to draw the game frames onto
    SDL_Texture* gameTexture;

    /// Initialize the graphical interface.
    void init() {
        // Initialize graphics system:
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

        // Initialize graphics structures:
        window = SDL_CreateWindow(
            "laines-py",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            WIDTH,
            HEIGHT,
            0 // SDL_WINDOW_HIDDEN
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

    }

    /// Send the PPU rendered frame to the SDL GUI.
    void new_frame(u32* pixels) {
        SDL_UpdateTexture(gameTexture, NULL, pixels, WIDTH * sizeof(u32));
    }

    /// Render the screen.
    void render() {
        // Clear the screen
        SDL_RenderClear(renderer);
        // Draw the NES screen:
        SDL_RenderCopy(renderer, gameTexture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

}
