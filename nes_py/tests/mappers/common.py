"""Shared helpers for synthetic mapper application tests."""
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

    def env(self, path, render_mode=None):
        """Create an environment and close it during test cleanup."""
        env = NESEnv(path, render_mode=render_mode)
        self.addCleanup(env.close)
        return env

    def assert_public_env_workflow(self, path):
        """Assert public NESEnv workflows operate for a ROM path."""
        env = self.env(path, render_mode='rgb_array')

        state, info = env.reset()
        self.assertEqual((240, 256, 3), state.shape)
        self.assertIsInstance(info, dict)
        state, reward, terminated, truncated, info = env.step(0)
        self.assertEqual((240, 256, 3), state.shape)
        self.assertIsInstance(reward, float)
        self.assertIsInstance(terminated, bool)
        self.assertIsInstance(truncated, bool)
        self.assertFalse(truncated)
        self.assertIsInstance(info, dict)
        self.assertEqual((240, 256, 3), env.render().shape)
