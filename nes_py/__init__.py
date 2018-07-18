"""The nes-py NES emulator for Python 2 & 3."""
from .nes_env import NesENV


# explicitly define the outward facing API of this package
__all__ = [NesENV.__name__]
