"""An OpenAI Gym space to convert MultiBinary to a standard bitmap."""
import numpy as np
from gym.spaces import MultiBinary


class Bitmap(MultiBinary):
    """A space for converting multi binary output to a bitmap."""

    def sample(self):
        """Return a sample from the space as an int."""
        return int(np.packbits(super(Bitmap, self).sample())[0])


# explicitly define the outward facing API for this module
__all__ = [Bitmap.__name__]
