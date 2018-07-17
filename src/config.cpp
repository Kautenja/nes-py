#include <cstdlib>
#include "config.hpp"
#include "gui.hpp"

namespace GUI {

    /* Controls settings */
    SDL_Scancode KEY_A     [] = { SDL_SCANCODE_A,      SDL_SCANCODE_ESCAPE };
    SDL_Scancode KEY_B     [] = { SDL_SCANCODE_S,      SDL_SCANCODE_ESCAPE };
    SDL_Scancode KEY_SELECT[] = { SDL_SCANCODE_SPACE,  SDL_SCANCODE_ESCAPE };
    SDL_Scancode KEY_START [] = { SDL_SCANCODE_RETURN, SDL_SCANCODE_ESCAPE };
    SDL_Scancode KEY_UP    [] = { SDL_SCANCODE_UP,     SDL_SCANCODE_ESCAPE };
    SDL_Scancode KEY_DOWN  [] = { SDL_SCANCODE_DOWN,   SDL_SCANCODE_ESCAPE };
    SDL_Scancode KEY_LEFT  [] = { SDL_SCANCODE_LEFT,   SDL_SCANCODE_ESCAPE };
    SDL_Scancode KEY_RIGHT [] = { SDL_SCANCODE_RIGHT,  SDL_SCANCODE_ESCAPE };
    int BTN_UP    [] = { -1, -1 };
    int BTN_DOWN  [] = { -1, -1 };
    int BTN_LEFT  [] = { -1, -1 };
    int BTN_RIGHT [] = { -1, -1 };
    int BTN_A     [] = { -1, -1 };
    int BTN_B     [] = { -1, -1 };
    int BTN_SELECT[] = { -1, -1 };
    int BTN_START [] = { -1, -1 };
    bool useJoystick[] = { false, false };

}
