"""A Python interface to the Cython native NES environment."""
import itertools
import gym
from gym.spaces import Box
from gym.spaces import Discrete
import numpy as np
from ._rom import ROM
from ._image_viewer import ImageViewer
from . import _native


def _native_cartridge_error(rom_path):
    """Return the native cartridge validation error for a ROM path, if any."""
    return _native.cartridge_error(rom_path)


def _native_cartridge_metadata(rom_path):
    """Return parsed native cartridge metadata for a ROM path."""
    return _native.cartridge_metadata(rom_path)


def _is_mapper_supported(mapper):
    """Return whether a mapper ID has a native implementation."""
    return _native.is_mapper_supported(mapper)


def _native_mapper_hook_smoke_results():
    """Run focused native mapper hook smoke checks."""
    return _native.mapper_hook_smoke_results()


def _native_mapper_bank_helper_smoke_results():
    """Run focused native mapper bank helper smoke checks."""
    return _native.mapper_bank_helper_smoke_results()


# height in pixels of the NES screen
SCREEN_HEIGHT = _native.SCREEN_HEIGHT
# width in pixels of the NES screen
SCREEN_WIDTH = _native.SCREEN_WIDTH
# shape of the screen as 24-bit RGB (standard for NumPy)
SCREEN_SHAPE_24_BIT = SCREEN_HEIGHT, SCREEN_WIDTH, 3
# shape of the screen as 32-bit RGB (C++ memory arrangement)
SCREEN_SHAPE_32_BIT = SCREEN_HEIGHT, SCREEN_WIDTH, 4


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

    def __init__(self, rom_path):
        """
        Create a new NES environment.

        Args:
            rom_path (str): the path to the ROM for the environment

        Returns:
            None

        """
        # create a ROM file from the ROM path
        rom = ROM(rom_path)
        # check that there is PRG ROM
        if rom.prg_rom_size == 0:
            raise ValueError('ROM has no PRG-ROM banks.')
        # ensure that there is no trainer
        if rom.has_trainer:
            raise ValueError('ROM has trainer. trainer is not supported.')
        # try to read the PRG ROM and raise a value error if it fails
        _ = rom.prg_rom
        # try to read the CHR ROM and raise a value error if it fails
        _ = rom.chr_rom
        # check the TV system
        if rom.is_pal:
            raise ValueError('ROM is PAL. PAL is not supported.')
        # check that the mapper is implemented
        elif not _is_mapper_supported(rom.mapper):
            msg = 'ROM has an unsupported mapper number {}. please see https://github.com/Kautenja/nes-py/issues/28 for more information.'
            raise ValueError(msg.format(rom.mapper))
        # create a dedicated random number generator for the environment
        self.np_random = np.random.RandomState()
        # store the ROM path
        self._rom_path = rom_path
        # initialize the C++ object for running the environment
        self._env = _native.NativeEmulator(self._rom_path)
        # setup a placeholder for a 'human' render mode viewer
        self.viewer = None
        # setup a placeholder for a pointer to a backup state
        self._has_backup = False
        # setup a done flag
        self.done = True
        # setup the controllers, screen, and RAM buffers
        self.controllers = [self._controller_buffer(port) for port in range(2)]
        self.screen = self._screen_buffer()
        self.ram = self._ram_buffer()

    def _screen_buffer(self):
        """Setup the screen buffer from the C++ code."""
        return self._env.screen_buffer()

    def _ram_buffer(self):
        """Setup the RAM buffer from the C++ code."""
        return self._env.ram_buffer()

    def _controller_buffer(self, port):
        """
        Find the pointer to a controller and setup a NumPy buffer.

        Args:
            port: the port of the controller to setup

        Returns:
            a NumPy buffer with the controller's binary data

        """
        return self._env.controller_buffer(port)

    def _mapper_number(self):
        """Return the active native mapper number."""
        return self._env.mapper_number()

    def _prg_rom_size(self):
        """Return the native PRG ROM size in bytes."""
        return self._env.prg_rom_size()

    def _chr_rom_size(self):
        """Return the native CHR ROM size in bytes."""
        return self._env.chr_rom_size()

    def _has_chr_ram(self):
        """Return whether the active native mapper uses CHR RAM."""
        return self._env.has_chr_ram()

    def _name_table_mirroring(self):
        """Return the active native mapper name table mirroring mode."""
        return self._env.name_table_mirroring()

    def _read_prg(self, address):
        """Read a byte through the active native PRG mapper."""
        return self._env.read_prg(address)

    def _write_prg(self, address, value):
        """Write a byte through the active native PRG mapper."""
        self._env.write_prg(address, value)

    def _read_chr(self, address):
        """Read a byte through the active native CHR mapper."""
        return self._env.read_chr(address)

    def _write_chr(self, address, value):
        """Write a byte through the active native CHR mapper."""
        self._env.write_chr(address, value)

    def _frame_advance(self, action):
        """
        Advance a frame in the emulator with an action.

        Args:
            action (byte): the action to press on the joy-pad

        Returns:
            None

        """
        self._env.frame_advance(action)

    def _backup(self):
        """Backup the NES state in the emulator."""
        self._env.backup()
        self._has_backup = True

    def _restore(self):
        """Restore the backup state into the NES emulator."""
        self._env.restore()

    def _will_reset(self):
        """Handle any RAM hacking after a reset occurs."""
        pass

    def seed(self, seed=None):
        """
        Set the seed for this environment's random number generator.

        Returns:
            list<bigint>: Returns the list of seeds used in this env's random
              number generators. The first value in the list should be the
              "main" seed, or the value which a reproducer should pass to
              'seed'. Often, the main seed equals the provided 'seed', but
              this won't be true if seed=None, for example.

        """
        # if there is no seed, return an empty list
        if seed is None:
            return []
        # set the random number seed for the NumPy random number generator
        self.np_random.seed(seed)
        # return the list of seeds used by RNG(s) in the environment
        return [seed]

    def reset(self, seed=None, options=None, return_info=None):
        """
        Reset the state of the environment and returns an initial observation.

        Args:
            seed (int): an optional random number seed for the next episode
            options (any): unused
            return_info (any): unused

        Returns:
            state (np.ndarray): next frame as a result of the given action

        """
        # Set the seed.
        self.seed(seed)
        # call the before reset callback
        self._will_reset()
        # reset the emulator
        if self._has_backup:
            self._restore()
        else:
            self._env.reset()
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
        # if the environment is done, raise an error
        if self.done:
            raise ValueError('cannot step in a done environment! call `reset`')
        # pass the action to the emulator as an unsigned byte
        self._env.frame_advance(action)
        # get the reward for this step
        reward = float(self._get_reward())
        # get the done flag for this step
        self.done = bool(self._get_done())
        # get the info for this step
        info = self._get_info()
        # call the after step callback
        self._did_step(self.done)
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
            done (bool): whether the done flag is set to true

        Returns:
            None

        """
        pass

    def close(self):
        """Close the environment."""
        # make sure the environment hasn't already been closed
        if self._env is None:
            raise ValueError('env has already been closed.')
        # close native operations
        self._env.close()
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

    def get_action_meanings(self):
        """Return a list of actions meanings."""
        return ['NOOP']


# explicitly define the outward facing API of this module
__all__ = [NESEnv.__name__]
