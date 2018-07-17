#pragma once
#include <cerrno>
#include <sys/stat.h>
#include <SDL2/SDL.h>

namespace GUI {

    extern int last_window_size;
    extern SDL_Scancode KEY_A     [];
    extern SDL_Scancode KEY_B     [];
    extern SDL_Scancode KEY_SELECT[];
    extern SDL_Scancode KEY_START [];
    extern SDL_Scancode KEY_UP    [];
    extern SDL_Scancode KEY_DOWN  [];
    extern SDL_Scancode KEY_LEFT  [];
    extern SDL_Scancode KEY_RIGHT [];
    extern int BTN_UP    [];
    extern int BTN_DOWN  [];
    extern int BTN_LEFT  [];
    extern int BTN_RIGHT [];
    extern int BTN_A     [];
    extern int BTN_B     [];
    extern int BTN_SELECT[];
    extern int BTN_START [];
    extern bool useJoystick[];

}
