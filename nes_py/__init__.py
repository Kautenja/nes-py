"""The nes-py NES emulator and Gymnasium environment package."""
from .nes_env import NESEnv


# explicitly define the outward facing API of this package
__all__ = [NESEnv.__name__]
