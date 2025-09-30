"""Wrappers for altering the functionality of the game."""
from .joypad_space import JoypadSpace


# explicitly define the outward facing API of this package
__all__ = [JoypadSpace.__name__]
