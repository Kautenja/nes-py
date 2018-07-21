"""A memory efficient buffer for frame tensors."""
import numpy as np


class LazyFrames(object):
    """A memory efficient buffer for frame tensors.

    Note:
        This object ensures that common frames between the observations are
        only stored once. It exists purely to optimize memory usage which can
        be huge for DQN's 1M frames replay buffers. This object should only be
        converted to numpy array before being passed to the model. You'd not
        believe how complex the previous solution was.

    """

    def __init__(self, frames):
        """
        Initialize a new lazy frames object.

        Args:
            frames (list): the list of frames to store lazily

        """
        self._frames = frames
        self._out = None

    def _force(self):
        """Force the internal buffer of frames into a NumPy array."""
        if self._out is None:
            self._out = np.concatenate(self._frames, axis=2)
            self._frames = None

        return self._out

    def __array__(self, dtype=None):
        """
        Convert this lazy frame buffer to a NumPy array.

        Args:
            dtype (numpy.dtype): the type to cast the member values to

        Returns:
            (numpy.ndarray) a NumPy array from the frames in this lazy buffer

        """
        out = self._force()
        if dtype is not None:
            out = out.astype(dtype)

        return out

    def __len__(self):
        """Return the number of frames in this lazy frame buffer."""
        return len(self._force())

    def __getitem__(self, index):
        """
        Return the frame at index i.

        Args:
            index (int): the index (or slice) of the frame to return

        Returns:
            (numpy.ndarray) the frame stored at index i

        """
        return self._force()[index]


# explicitly define the outward facing API of this module
__all__ = [LazyFrames.__name__]
