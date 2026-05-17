"""Shared helpers for mapper package tests."""
import tempfile
from unittest import TestCase

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

    def env(self, path):
        """Create an environment and close it during test cleanup."""
        env = NESEnv(path)
        self.addCleanup(self._close_env, env)
        return env

    def _close_env(self, env):
        """Close an environment if it is still open."""
        if env._env is not None:
            env.close()

    def assert_env_smoke(self, path):
        """Assert public NESEnv workflows operate for a ROM path."""
        env = self.env(path)

        state = env.reset()
        self.assertEqual((240, 256, 3), state.shape)
        state, reward, done, info = env.step(0)
        self.assertEqual((240, 256, 3), state.shape)
        self.assertIsInstance(reward, float)
        self.assertIsInstance(done, bool)
        self.assertIsInstance(info, dict)
        self.assertEqual((240, 256, 3), env.render('rgb_array').shape)

        env._backup()
        env.step(0)
        env._restore()
        env.done = False
        state, _, _, _ = env.step(0)
        self.assertEqual((240, 256, 3), state.shape)
