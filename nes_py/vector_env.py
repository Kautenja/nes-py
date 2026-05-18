"""Opt-in native vector emulator helpers for same-ROM NES workloads."""
import time
from dataclasses import asdict
from dataclasses import dataclass

import numpy as np
from gymnasium.spaces import Box
from gymnasium.spaces import Discrete

from . import _native
from ._rom import ROM
from .nes_env import OBSERVATION_MODE_GRAYSCALE
from .nes_env import OBSERVATION_MODE_RGB_ARRAY
from .nes_env import OBSERVATION_MODE_RGB_ARRAY_CONTIGUOUS
from .nes_env import SCREEN_SHAPE_24_BIT
from .nes_env import SCREEN_SHAPE_GRAYSCALE
from .nes_env import _is_mapper_supported
from .ram import normalize_ram_read_specs


@dataclass(frozen=True)
class VectorStepTiming:
    """Timing counters for one vector step call."""

    python_overhead_seconds: float
    action_transfer_seconds: float
    native_step_seconds: float
    synchronization_seconds: float
    observation_seconds: float = 0.0
    ram_info_seconds: float = 0.0
    worker_stats: tuple = ()

    def to_dict(self):
        """Return this timing result as a JSON-serializable dictionary."""
        return asdict(self)


def _validate_rom(rom_path):
    """Apply the same generic ROM checks used by ``NESEnv``."""
    rom = ROM(rom_path)
    if rom.prg_rom_size == 0:
        raise ValueError('ROM has no PRG-ROM banks.')
    if rom.has_trainer:
        raise ValueError('ROM has trainer. trainer is not supported.')
    _ = rom.prg_rom
    _ = rom.chr_rom
    if rom.is_pal:
        raise ValueError('ROM is PAL. PAL is not supported.')
    if not _is_mapper_supported(rom.mapper):
        msg = (
            'ROM has an unsupported mapper number {}. please see '
            'https://github.com/Kautenja/nes-py/issues/28 for more '
            'information.'
        )
        raise ValueError(msg.format(rom.mapper))


class VectorNESEmulator:
    """
    Same-ROM native vector emulator.

    This class intentionally keeps game-specific reward, termination, and info
    logic out of ``nes-py``. It batches controller writes, frame advances,
    observation copies, RAM reads, resets, and opaque state snapshots for
    wrapper or training-loop code that already owns those semantics.
    """

    action_space = Discrete(256)
    observation_space = Box(
        low=0,
        high=255,
        shape=SCREEN_SHAPE_24_BIT,
        dtype=np.uint8,
    )

    def __init__(self, rom_path, env_count):
        """Create ``env_count`` native emulator instances for one ROM."""
        _validate_rom(rom_path)
        self.rom_path = rom_path
        self.num_envs = int(env_count)
        if self.num_envs <= 0:
            raise ValueError('env_count must be positive')
        self._native = _native.NativeVectorEmulator(rom_path, self.num_envs)
        self.screens = tuple(
            self._native.screen_buffer(index)
            for index in range(self.num_envs)
        )
        self.rams = tuple(
            self._native.ram_buffer(index)
            for index in range(self.num_envs)
        )

    def _require_open(self):
        """Raise if the native vector emulator is closed."""
        if self._native is None:
            raise ValueError('vector emulator has already been closed.')

    def close(self):
        """Close native operations while existing buffer views remain valid."""
        self._require_open()
        self._native.close()
        self._native = None

    def reset(self, *, seed=None):
        """Reset all vector slots and return the zero-copy screen views."""
        del seed
        self._require_open()
        self._native.reset()
        return self.observation()

    def reset_one(self, index):
        """Reset one vector slot and return its zero-copy screen view."""
        self._require_open()
        self._native.reset_one(index)
        return self.screens[int(index)]

    def step(self, actions):
        """Step all vector slots with a one-dimensional uint8 action array."""
        self._require_open()
        self._native.step(actions)
        return self.observation()

    def step_timed(self, actions):
        """Step all vector slots and return timing counters."""
        self._require_open()
        started_at = time.perf_counter()
        array = np.asarray(actions)
        transferred_at = time.perf_counter()
        self._native.step(array)
        stepped_at = time.perf_counter()
        native_seconds = stepped_at - transferred_at
        total_seconds = stepped_at - started_at
        action_seconds = transferred_at - started_at
        return VectorStepTiming(
            python_overhead_seconds=max(
                0.0,
                total_seconds - native_seconds - action_seconds,
            ),
            action_transfer_seconds=action_seconds,
            native_step_seconds=native_seconds,
            synchronization_seconds=0.0,
            worker_stats=(),
        )

    def step_one(self, index, action):
        """Step one vector slot and return its zero-copy screen view."""
        self._require_open()
        self._native.step_one(index, action)
        return self.screens[int(index)]

    def observation(self, mode=OBSERVATION_MODE_RGB_ARRAY, output=None):
        """Return vector observations using zero-copy or native copy modes."""
        if mode == OBSERVATION_MODE_RGB_ARRAY:
            return self.screens
        self._require_open()
        if mode == OBSERVATION_MODE_RGB_ARRAY_CONTIGUOUS:
            return self._native.copy_screen_rgb_batch(output)
        if mode == OBSERVATION_MODE_GRAYSCALE:
            return self._native.copy_screen_grayscale_batch(output)
        modes = (
            OBSERVATION_MODE_RGB_ARRAY,
            OBSERVATION_MODE_RGB_ARRAY_CONTIGUOUS,
            OBSERVATION_MODE_GRAYSCALE,
        )
        msg = 'valid observation modes are: {}'.format(
            ', '.join(repr(mode) for mode in modes)
        )
        raise NotImplementedError(msg)

    def observation_one(
        self,
        index,
        mode=OBSERVATION_MODE_RGB_ARRAY,
        output=None,
    ):
        """Return one vector slot observation."""
        env_index = int(index)
        if env_index < 0 or env_index >= self.num_envs:
            raise IndexError('env index out of range')
        if mode == OBSERVATION_MODE_RGB_ARRAY:
            return self.screens[env_index]
        self._require_open()
        if mode == OBSERVATION_MODE_RGB_ARRAY_CONTIGUOUS:
            return self._native.copy_screen_rgb(index, output)
        if mode == OBSERVATION_MODE_GRAYSCALE:
            return self._native.copy_screen_grayscale(index, output)
        raise NotImplementedError('unknown observation mode: {}'.format(mode))

    def ram_values(self, specs, output=None):
        """Read configured RAM values for every vector slot."""
        self._require_open()
        addresses, sizes, encodings = normalize_ram_read_specs(specs)
        return self._native.read_ram_values(
            addresses,
            sizes,
            encodings,
            output,
        )

    def dump_state(self, index):
        """Return an opaque state snapshot for one vector slot."""
        self._require_open()
        return self._native.dump_state(index)

    def load_state(self, index, snapshot):
        """Restore one vector slot from an opaque state snapshot."""
        self._require_open()
        self._native.load_state(index, snapshot)


__all__ = [
    'VectorNESEmulator',
    'VectorStepTiming',
]
