"""A CTypes interface to the C++ NES environment."""
import ctypes
import glob
import itertools
import os
import sys
import gym
from gym.spaces import Box
from gym.spaces import Discrete
import numpy as np


# the path to the directory this file is in
_MODULE_PATH = os.path.dirname(__file__)
# the pattern to find the C++ shared object library
_SO_PATH = 'lib_nes_env*'
# the absolute path to the C++ shared object library
_LIB_PATH = os.path.join(_MODULE_PATH, _SO_PATH)
# load the library from the shared object file
try:
    _LIB = ctypes.cdll.LoadLibrary(glob.glob(_LIB_PATH)[0])
except IndexError:
    raise OSError('missing static lib_nes_env*.so library!')


# setup the argument and return types for GetNESWidth
_LIB.GetNESWidth.argtypes = None
_LIB.GetNESWidth.restype = ctypes.c_uint
# setup the argument and return types for GetNESHeight
_LIB.GetNESHeight.argtypes = None
_LIB.GetNESHeight.restype = ctypes.c_uint
# setup the argument and return types for Initialize
_LIB.Initialize.argtypes = [ctypes.c_wchar_p]
_LIB.Initialize.restype = ctypes.c_void_p
# setup the argument and return types for Screen
_LIB.Screen.argtypes = [ctypes.c_void_p]
_LIB.Screen.restype = ctypes.c_void_p
# setup the argument and return types for GetMemoryBuffer
_LIB.Memory.argtypes = [ctypes.c_void_p]
_LIB.Memory.restype = ctypes.c_void_p
# setup the argument and return types for Reset
_LIB.Reset.argtypes = [ctypes.c_void_p]
_LIB.Reset.restype = None
# setup the argument and return types for Step
_LIB.Step.argtypes = [ctypes.c_void_p, ctypes.c_ubyte]
_LIB.Step.restype = None
# setup the argument and return types for Backup
_LIB.Backup.argtypes = [ctypes.c_void_p]
_LIB.Backup.restype = None
# setup the argument and return types for Restore
_LIB.Restore.argtypes = [ctypes.c_void_p]
_LIB.Restore.restype = None
# setup the argument and return types for Close
_LIB.Close.argtypes = [ctypes.c_void_p]
_LIB.Close.restype = None


# height in pixels of the NES screen
SCREEN_HEIGHT = _LIB.GetNESHeight()
# width in pixels of the NES screen
SCREEN_WIDTH = _LIB.GetNESWidth()
# shape of the screen as 24-bit RGB (standard for NumPy)
SCREEN_SHAPE_24_BIT = SCREEN_HEIGHT, SCREEN_WIDTH, 3
# shape of the screen as 32-bit RGB (C++ memory arrangement)
SCREEN_SHAPE_32_BIT = SCREEN_HEIGHT, SCREEN_WIDTH, 4
# create a type for the screen tensor matrix from C++
SCREEN_TENSOR = ctypes.c_byte * np.prod(SCREEN_SHAPE_32_BIT)


# create a type for the RAM vector from C++
RAM_VECTOR = ctypes.c_byte * 0x800


# the magic bytes expected at the first four bytes of the iNES ROM header.
# It spells "NES<END>"
MAGIC = bytearray([0x4E, 0x45, 0x53, 0x1A])


class NESEnv(gym.Env):
    """An NES environment based on the LaiNES emulator."""

    # relevant meta-data about the environment
    metadata = {
        'render.modes': ['rgb_array', 'human'],
        'video.frames_per_second': 60
    }

    # the legal range for rewards for this environment
    reward_range = (-float('inf'), float('inf'))

    # observation space for the environment is static across all instances
    observation_space = Box(
        low=0,
        high=255,
        shape=SCREEN_SHAPE_24_BIT,
        dtype=np.uint8
    )

    # action space is a bitmap of button press values for the 8 NES buttons
    action_space = Discrete(256)

    def __init__(self, rom_path,
        frames_per_step=1,
        max_episode_steps=float('inf')
    ):
        """
        Create a new NES environment.

        Args:
            rom_path (str): the path to the ROM for the environment
            frames_per_step (int): the number of frames between steps
            max_episode_steps (int): number of steps before an episode ends

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
            magic = nes_file.read(4)
        if magic != MAGIC:
            raise ValueError('{} is not a valid ".nes" file'.format(rom_path))
        self._rom_path = rom_path

        # check the frame skip variable
        if not isinstance(frames_per_step, int):
            raise TypeError('frames_per_step must be of type: int')
        if frames_per_step <= 0:
            raise ValueError('frames_per_step must be > 0')
        self._frames_per_step = frames_per_step
        # adjust the FPS of the environment by the given frames_per_step value
        self.metadata['video.frames_per_second'] /= frames_per_step

        # check the max episode steps
        if not isinstance(max_episode_steps, (int, float)):
            raise TypeError('max_episode_steps must be of type: int, float')
        if max_episode_steps <= 0:
            raise ValueError('max_episode_steps must be > 0')
        self._max_episode_steps = max_episode_steps
        self._steps = 0

        # initialize the C++ object for running the environment
        self._env = _LIB.Initialize(self._rom_path)
        # setup a placeholder for a 'human' render mode viewer
        self.viewer = None
        # setup a placeholder for a pointer to a backup state
        self._backup_env = None
        # setup the NumPy screen from the C++ vector
        self.screen = None
        self.ram = None
        self.done = True
        self._setup_screen()
        self._setup_ram()

    def _setup_screen(self):
        """Setup the screen buffer from the C++ code."""
        # get the address of the screen
        address = _LIB.Screen(self._env)
        # create a buffer from the contents of the address location
        buffer_ = ctypes.cast(address, ctypes.POINTER(SCREEN_TENSOR)).contents
        # create a NumPy array from the buffer
        screen = np.frombuffer(buffer_, dtype='uint8')
        # reshape the screen from a column vector to a tensor
        screen = screen.reshape(SCREEN_SHAPE_32_BIT)
        # flip the bytes if the machine is little-endian (which it likely is)
        if sys.byteorder == 'little':
            # invert the little-endian BGR channels to RGB
            screen = screen[:, :, ::-1]
        # remove the 0th axis (padding from storing colors in 32 bit)
        screen = screen[:, :, 1:]
        # store the instance to screen
        self.screen = screen

    def _setup_ram(self):
        """Setup the RAM buffer from the C++ code."""
        # get the address of the RAM
        address = _LIB.Memory(self._env)
        # create a buffer from the contents of the address location
        buffer_ = ctypes.cast(address, ctypes.POINTER(RAM_VECTOR)).contents
        # create a NumPy array from the buffer
        self.ram = np.frombuffer(buffer_, dtype='uint8')
        # TODO: handle endian-ness of machine?

    def _frame_advance(self, action):
        """
        Advance a frame in the emulator with an action.

        Args:
            action: the action to press on the joy-pad

        Returns:
            None

        """
        _LIB.Step(self._env, action)

    def _backup(self):
        """Backup the NES state in the emulator."""
        _LIB.Backup(self._env)

    def _restore(self):
        """Restore the backup state into the NES emulator."""
        _LIB.Restore(self._env)

    def _will_reset(self):
        """Handle any RAM hacking after a reset occurs."""
        pass

    def reset(self):
        """
        Reset the state of the environment and returns an initial observation.

        Returns:
            state (np.ndarray): next frame as a result of the given action

        """
        # reset the steps counter
        self._steps = 0
        # call the before reset callback
        self._will_reset()
        # reset the emulator
        if self._backup_env is not None:
            self._restore()
        else:
            _LIB.Reset(self._env)
        # call the after reset callback
        self._did_reset()
        # set the done flag to false
        self.done = False
        # return the screen from the emulator
        return self.screen

    def _did_reset(self):
        """Handle any RAM hacking after a reset occurs."""
        pass

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
        if self.done:
            raise ValueError('cannot step in a done environment! call `reset`')
        # setup the reward, done, and info for this step
        reward = 0
        self.done = False
        info = {}
        # iterate over the frames to skip
        for _ in range(self._frames_per_step):
            # pass the action to the emulator as an unsigned byte
            _LIB.Step(self._env, action)
            # get the reward for this step
            reward += self._get_reward()
            # get the done flag for this step
            self.done = self.done or self._get_done()
            # get the info for this step
            info = self._get_info()
            # if done terminate the collection early
            if self.done:
                break
        # call the after step callback
        self._did_step(self.done)
        # increment the steps counter
        self._steps += 1
        # set the done flag to true if the steps are past the max
        if self._steps >= self._max_episode_steps:
            self.done = True
        # bound the reward in [min, max]
        if reward < self.reward_range[0]:
            reward = self.reward_range[0]
        elif reward > self.reward_range[1]:
            reward = self.reward_range[1]
        # return the screen from the emulator and other relevant data
        return self.screen, reward, self.done, info

    def _get_reward(self):
        """Return the reward after a step occurs."""
        return 0

    def _get_done(self):
        """Return True if the episode is over, False otherwise."""
        return False

    def _get_info(self):
        """Return the info after a step occurs."""
        return {}

    def _did_step(self, done):
        """
        Handle any RAM hacking after a step occurs.

        Args:
            done: whether the done flag is set to true

        Returns:
            None

        """
        pass

    def close(self):
        """Close the environment."""
        # make sure the environment hasn't already been closed
        if self._env is None:
            raise ValueError('env has already been closed.')
        # purge the environment from C++ memory
        _LIB.Close(self._env)
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
