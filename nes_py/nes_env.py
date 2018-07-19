"""A CTypes interface to the C++ NES environment."""
import os
import sys
import ctypes
import itertools
from glob import glob
import gym
import numpy as np
from numpy.ctypeslib import as_ctypes
from .spaces import Bitmap


# the path to the directory this
_MODULE_PATH = os.path.dirname(__file__)
# the relative path to the C++ shared object library. it can be built on
# many different systems so all we know is the defined prefix
_SO_PATH = 'laines/build/lib_nes_env*'
# the absolute path to the C++ shared object library
_LIB_PATH = os.path.join(_MODULE_PATH, _SO_PATH)
# load the library from the shared object file. use a glob to locate the
# .so file based on the build systems nomenclature
_LIB = ctypes.cdll.LoadLibrary(glob(_LIB_PATH)[0])


# setup the argument and return types for NESEnv_init
_LIB.NESEnv_init.argtypes = [ctypes.c_wchar_p]
_LIB.NESEnv_init.restype = ctypes.c_void_p
# setup the argument and return types for NESEnv_num_buttons
_LIB.NESEnv_num_buttons.argtypes = None
_LIB.NESEnv_num_buttons.restype = ctypes.c_uint
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


# the number of buttons on the NES joypad
NUM_BUTTONS = _LIB.NESEnv_num_buttons()
# height in pixels of the NES screen
SCREEN_HEIGHT = _LIB.NESEnv_height()
# width in pixels of the NES screen
SCREEN_WIDTH = _LIB.NESEnv_width()
# shape of the screen as 24-bit RGB (standard for NumPy)
SCREEN_SHAPE_24_BIT = SCREEN_HEIGHT, SCREEN_WIDTH, 3
# shape of the screen as 32-bit RGB (C++ memory arrangement)
SCREEN_SHAPE_32_BIT = SCREEN_HEIGHT, SCREEN_WIDTH, 4


# the magic bytes expected at the first four bytes of the iNES ROM header.
# It spells "NES<END>"
MAGIC = bytes([0x4E, 0x45, 0x53, 0x1A])


class NESEnv(gym.Env):
    """An NES environment based on the LaiNES emulator."""

    # relevant metadata about the environment
    metadata = {
        'render.modes': ['rgb_array', 'human'],
        'video.frames_per_second': 60
    }

    # observation space for the environment is static across all instances
    observation_space = gym.spaces.Box(
        low=0,
        high=255,
        shape=SCREEN_SHAPE_24_BIT,
        dtype=np.uint8
    )

    # action space is a bitmap of button press values for the 8 NES buttons
    action_space = Bitmap(NUM_BUTTONS)

    def __init__(self, rom_path):
        """
        Create a new NES environment.

        Args:
            path (str): the path to the ROM for the environment

        Returns:
            None

        """
        # ensure that rom_path is a string
        if not isinstance(rom_path, str):
            raise TypeError('rom_path should be of type: str')
        # ensure that rom_path points to an existing .nes file
        if not '.nes' in rom_path or not os.path.isfile(rom_path):
            raise ValueError('rom_path should point to a ".nes" file')
        # make sure the magic characters are in the iNES file
        with open(rom_path, 'rb') as nes_file:
            raw_data = nes_file.read()
            magic = raw_data[:4]
        if magic != MAGIC:
            raise ValueError('{} is not a valid ".nes" file'.format(rom_path))
        self._rom_path = rom_path
        # initialize the C++ object for running the environment
        self._env = _LIB.NESEnv_init(self._rom_path)
        # setup a boolean for whether to flip from BGR to RGB based on machine
        # byte order
        self._is_little_endian = sys.byteorder == 'little'
        # setup a placeholder for a 'human' render mode viewer
        self.viewer = None
        # create a frame for the screen data (32-bit format from C++)
        self._screen_data = np.empty(SCREEN_SHAPE_32_BIT, dtype=np.uint8)
        # setup the screen for the environment (24-bit RGB format for Python)
        self.screen = np.empty(SCREEN_SHAPE_24_BIT, dtype=np.uint8)

    def _copy_screen(self):
        """Copy screen data from the C++ shared object library."""
        # fill the screen data array with values from the emulator
        _LIB.NESEnv_screen(as_ctypes(self._screen_data))
        # copy the screen data to the screen
        self.screen = self._screen_data
        # flip the bytes if the machine is little endian (which it likely is)
        if self._is_little_endian:
            # invert the little-endian BGR channels to RGB
            self.screen = self.screen[:, :, ::-1]
        # remove the 0th axis (padding from storing colors in 32 bit)
        self.screen = self.screen[:, :, 1:]

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
        # copy the screen from the emulator
        self._copy_screen()
        # return the screen from the emulator
        return self.screen

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
        _LIB.NESEnv_step(self._env, action)
        # copy the screen from the emulator
        self._copy_screen()
        # return the screen from the emulator and other relevant data
        return self.screen, self._reward, self._done, {}

    def close(self):
        """Close the environment."""
        # make sure the environment hasn't already been closed
        if self._env is None:
            raise ValueError('env has already been closed.')
        # purge the environment from C++ memory
        _LIB.NESEnv_close(self._env)
        # deallocate the object locally
        self._env = None
        # if there is an image viewer open, delete it
        if self.viewer is not None:
            self.viewer.close()

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
            # if the viewer isn't setup, import it and create one
            if self.viewer is None:
                from ._image_viewer import ImageViewer
                # get the caption for the ImageViewer
                if self.spec is None:
                    # if there is no spec, just use the .nes filename
                    caption = self._rom_path.split('/')[-1]
                else:
                    # set the caption to the OpenAI Gym id
                    caption = self.spec.id
                # create the ImageViewer to display frames
                self.viewer = ImageViewer(
                    caption=caption,
                    height=SCREEN_HEIGHT,
                    width=SCREEN_WIDTH,
                )
            # show the screen on the image viewer
            self.viewer.show(self.screen)
        elif mode == 'rgb_array':
            return self.screen
        else:
            # unpack the modes as comma delineated strings ('a', 'b', ...)
            render_modes = [repr(x) for x in self.metadata['render.modes']]
            msg = 'valid render modes are: {}'.format(', '.join(render_modes))
            raise NotImplementedError(msg)

    def get_keys_to_action(self):
        """Return the dictionary of keyboard keys to actions."""
        # keyboard keys in an array ordered by their byte order in the bitmap
        # i.e. right = 7, left = 6, ..., B = 1, A = 0
        buttons = np.array([
            ord('d'),  # right
            ord('a'),  # left
            ord('s'),  # down
            ord('w'),  # up
            ord('\r'), # start
            ord(' '),  # select
            ord('p'),  # B
            ord('o'),  # A
        ])
        # the dictionary of key presses to controller codes
        keys_to_action = {}
        # the combination map of values for the controller
        values = 8 * [[0, 1]]
        # iterate over all the combinations
        for combination in itertools.product(*values):
            # unpack the tuple of bits into an integer
            byte = int(''.join(map(str, combination)), 2)
            # unwrap the pressed buttons based on the bitmap
            pressed = buttons[list(map(bool, combination))]
            # assign the pressed buttons to the output byte
            keys_to_action[tuple(sorted(pressed))] = byte

        return keys_to_action


# explicitly define the outward facing API of this module
__all__ = [NESEnv.__name__]
