"""A CTypes interface to the C++ NES environment."""
import sys
import ctypes
import gym
import numpy as np
from numpy.ctypeslib import as_ctypes


# load the library from the shared object file
_LIB = ctypes.cdll.LoadLibrary('build/lib_nes_env.so')
# setup the argument and return types for NESEnv_init
_LIB.NESEnv_init.argtypes = [ctypes.c_wchar_p]
_LIB.NESEnv_init.restype = ctypes.c_void_p
# setup the argument and return types for NESEnv_width
_LIB.NESEnv_width.argtypes = None
_LIB.NESEnv_width.restype = ctypes.c_uint
# setup the argument and return types for NESEnv_height
_LIB.NESEnv_height.argtypes = None
_LIB.NESEnv_height.restype = ctypes.c_uint
# setup the argument and return types for NESEnv_screen
_LIB.NESEnv_screen.argtypes = [ctypes.c_void_p]
_LIB.NESEnv_screen.restype = None
# setup the argument and return types for NESEnv_reset
_LIB.NESEnv_reset.argtypes = [ctypes.c_void_p]
_LIB.NESEnv_reset.restype = None
# setup the argument and return types for NESEnv_step
_LIB.NESEnv_step.argtypes = [ctypes.c_ubyte]
_LIB.NESEnv_step.restype = None
# setup the argument and return types for NESEnv_close
_LIB.NESEnv_close.argtypes = [ctypes.c_void_p]
_LIB.NESEnv_close.restype = None


# the height in pixels of the NES screen
SCREEN_HEIGHT = _LIB.NESEnv_height()
# the width in pixels of the NES screen
SCREEN_WIDTH = _LIB.NESEnv_width()
# the shape of the screen as 24-bit RGB (standard for NumPy)
SCREEN_SHAPE_24_BIT = SCREEN_HEIGHT, SCREEN_WIDTH, 3
# the shape of the screen as 32-bit RGB (C++ memory arrangement)
SCREEN_SHAPE_32_BIT = SCREEN_HEIGHT, SCREEN_WIDTH, 4


class NesENV(gym.Env):
    """An NES environment based on the LaiNES emulator."""

    # relevant metadata about the environment
    metadata = { 'render.modes': ['rgb_array', 'human'] }

    # the observation space for the environment is static across all instances
    observation_space = gym.spaces.Box(
        low=0,
        high=255,
        shape=SCREEN_SHAPE_24_BIT,
        dtype=np.uint8
    )

    def __init__(self, rom_path):
        """
        Create a new NES environment.

        Args:
            path (str): the path to the ROM for the environment

        Returns:
            None

        """
        self._rom_path = rom_path
        # initialize the C++ object for running the environment
        self._env = _LIB.NESEnv_init(self._rom_path)
        # setup a boolean for whether to flip from BGR to RGB based on machine
        # byte order
        self._is_little_endian = sys.byteorder == 'little'

    @property
    def _screen(self):
        """
        Return screen data in RGB format.

        Note:
            -   On little-endian machines like x86, the channels are BGR
                order: screen_data[x, y, 0:3] is [blue, green, red]
            -   On big-endian machines (rare in 2017) the channels would be
                the opposite order.

        Returns:
            a numpy array with the screen's RGB data

        """
        # create a frame for the screen data
        screen_data = np.empty(SCREEN_SHAPE_32_BIT, dtype=np.uint8)
        # fill the screen data array with values from the emulator
        _LIB.NESEnv_screen(as_ctypes(screen_data[:]))

        # flip the bytes if the machine is little endian (which it likely is)
        if self._is_little_endian:
            # invert the little-endian BGR channels to RGB
            screen_data = screen_data[:, :, ::-1]

        # remove the 0th axis (padding from storing colors in 32 bit)
        screen_data = screen_data[:, :, 1:]

        from PIL import Image
        Image.fromarray(screen_data).save('screen.png')

        return screen_data

    @property
    def _reward(self):
        """
        """
        return 0

    @property
    def _done(self):
        """
        """
        return False

    def reset(self):
        """
        Reset the state of the environment and returns an initial observation.

        Returns:
            state (np.ndarray): next frame as a result of the given action

        """
        # reset the emulator
        _LIB.NESEnv_reset(self._env)
        # return the screen from the emulator
        return self._screen

    def step(self, action):
        """
        Run one frame of the NES and return the relevant observation data.

        Args:
            action (byte): the bitmap determining which buttons to press

        Returns:
            a tuple of:
            - state (np.ndarray): next frame as a result of the given action
            - reward (float) : amount of reward returned after given action
            - done (boolean): whether the episode has ended
            - info (dict): contains auxiliary diagnostic information

        """
        # pass the action to the emulator as an unsigned byte
        _LIB.NESEnv_step(self._env, ctypes.c_ubyte(action))
        # return the screen from the emulator and other relevant data
        return self._screen, self._reward, self._done, {}

    def close(self):
        """Close the environment."""
        # make sure the environment hasn't already been closed
        if self._env is None:
            raise ValueError('env has already been closed.')
        # purge the environment from C++ memory
        _LIB.NESEnv_close(self._env)
        # deallocate the object locally
        self._env = None

    def render(self, mode='human'):
        """
        Render the environment.

        Args:
            mode (str): the mode to render with:
            - human: render to the current display
            - rgb_array: Return an numpy.ndarray with shape (x, y, 3),
              representing RGB values for an x-by-y pixel image

        Returns:
            a numpy array if mode is 'rgb_array', None otherwise

        """
        if mode == 'human':
            raise NotImplementedError('TODO: human mode')
        elif mode == 'rgb_array':
            raise NotImplementedError('TODO: rgb_array mode')
        else:
            render_modes = self.metadata['render.modes']
            msg = 'valid render modes are {}'.format(render_modes)
            raise NotImplementedError(msg)


# explicitly define the outward facing API of this module
__all__ = [NesENV.__name__]


# handle running this environment as the main script
if __name__ == '__main__':
    from tqdm import tqdm
    path = sys.argv[1]

    # create the environment
    env = NesENV(path)
    # run through some random steps in the environment
    try:
        done = True
        for _ in tqdm(range(500)):
            if done:
                _ = env.reset()
            action = 8 # env.action_space.sample()
            _, reward, done, _ = env.step(action)
            # env.render()
    except KeyboardInterrupt:
        pass
    # close the environment
    env.close()
