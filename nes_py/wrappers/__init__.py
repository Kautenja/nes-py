"""Wrappers for altering the functionality of the game."""
from .binary_to_discrete_space_env import BinarySpaceToDiscreteSpaceEnv


# explicitly define the outward facing API of this package
__all__ = [BinarySpaceToDiscreteSpaceEnv.__name__]
