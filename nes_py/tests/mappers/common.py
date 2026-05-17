"""Shared helpers for synthetic mapper application tests."""
import tempfile
from unittest import TestCase

import numpy as np

from nes_py.nes_env import SCREEN_SHAPE_24_BIT
from nes_py.nes_env import NESEnv

from nes_py.tests.mapper_fixtures import synthetic_rom_path


HORIZONTAL = 0
VERTICAL = 1
ONE_SCREEN_LOWER = 9
ONE_SCREEN_HIGHER = 10


class MapperTestCase(TestCase):
    """Base test case that owns synthetic ROM temporary files."""

    def setUp(self):
        """Create a temporary directory for synthetic ROMs."""
        self.tmpdir = tempfile.TemporaryDirectory()

    def tearDown(self):
        """Remove temporary ROM files."""
        self.tmpdir.cleanup()

    def synthetic_rom(self, *args, **kwargs):
        """Create a synthetic ROM in this test's temporary directory."""
        return synthetic_rom_path(self.tmpdir.name, *args, **kwargs)

    def env(self, path, render_mode=None):
        """Create an environment and close it during test cleanup."""
        env = NESEnv(path, render_mode=render_mode)
        self.addCleanup(env.close)
        return env

    def assert_valid_frame(self, frame):
        """Assert an RGB frame matches the public NESEnv contract."""
        self.assertIsInstance(frame, np.ndarray)
        self.assertEqual(SCREEN_SHAPE_24_BIT, frame.shape)
        self.assertEqual(np.uint8, frame.dtype)

    def step_and_capture(self, env, actions):
        """Advance deterministic actions and copy public step outputs."""
        outputs = []
        for action in actions:
            state, reward, terminated, truncated, info = env.step(action)
            self.assert_valid_frame(state)
            self.assertIsInstance(reward, float)
            self.assertIsInstance(terminated, bool)
            self.assertIsInstance(truncated, bool)
            self.assertFalse(truncated)
            self.assertIsInstance(info, dict)
            outputs.append((
                state.copy(),
                reward,
                terminated,
                truncated,
                info.copy(),
            ))
        return outputs

    def assert_backup_restore_workflow(self, env, actions):
        """Assert backup/restore replays the same public step outputs."""
        env._backup()
        expected = self.step_and_capture(env, actions)
        env._restore()
        actual = self.step_and_capture(env, actions)

        for expected_output, actual_output in zip(expected, actual):
            self.assertTrue(np.array_equal(
                expected_output[0],
                actual_output[0],
            ))
            self.assertEqual(expected_output[1:], actual_output[1:])

    def assert_public_env_workflow(self, path):
        """Assert public NESEnv workflows operate for a ROM path."""
        env = self.env(path, render_mode='rgb_array')

        state, info = env.reset()
        self.assert_valid_frame(state)
        self.assertIsInstance(info, dict)
        state, reward, terminated, truncated, info = env.step(0)
        self.assert_valid_frame(state)
        self.assertIsInstance(reward, float)
        self.assertIsInstance(terminated, bool)
        self.assertIsInstance(truncated, bool)
        self.assertFalse(truncated)
        self.assertIsInstance(info, dict)
        self.assert_valid_frame(env.render())
