"""Mapper 009 / MMC2 public environment tests.

The representative-title integration test uses only a local, legally supplied
ROM fixture at ``nes_py/tests/games/mike-tysons-punch-out.nes``. It never
fetches ROMs from the network.
"""

import os

import numpy as np

from nes_py._rom import ROM
from nes_py.nes_env import SCREEN_SHAPE_24_BIT
from nes_py.tests.mappers.common import MapperTestCase
from nes_py.tests.rom_file_abs_path import rom_file_abs_path


REPRESENTATIVE_TITLE = "Mike Tyson's Punch-Out!! (USA)"
REPRESENTATIVE_ROM = rom_file_abs_path('mike-tysons-punch-out.nes')
DETERMINISTIC_ACTIONS = (0, 1, 2, 4, 8, 16, 32, 64)
CONTINUATION_ACTIONS = (255, 128, 64, 32, 16, 8, 4, 2)


class ShouldLoadMapper009MMC2(MapperTestCase):
    """Exercise MMC2 through the package API."""

    def test_chr_rom_fixture_constructs_and_steps(self):
        path = self.synthetic_rom(
            'mmc2-chr-rom.nes',
            mapper=9,
            prg_banks=4,
            chr_banks=8,
            chr_4k_markers=True,
            reset_vector=0x8000,
        )

        rom = ROM(path)
        self.assertEqual(9, rom.mapper)
        self.assertEqual(64, rom.prg_rom_size)
        self.assertEqual(64, rom.chr_rom_size)
        self.assert_public_env_workflow(path)


class ShouldExerciseRepresentativeMapper009Title(MapperTestCase):
    """Exercise the representative MMC2 title through the package API."""

    def require_representative_rom(self):
        """Return the local representative ROM path or skip narrowly."""
        if not os.path.exists(REPRESENTATIVE_ROM):
            message = (
                'place a legally owned {} dump at {} to run this '
                'representative mapper 009 integration test; the test never '
                'downloads commercial ROMs'
            )
            self.skipTest(message.format(
                REPRESENTATIVE_TITLE,
                REPRESENTATIVE_ROM,
            ))
        return REPRESENTATIVE_ROM

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

    def test_local_punch_out_fixture_exercises_env_render_and_backup_restore(self):
        path = self.require_representative_rom()
        rom = ROM(path)
        self.assertEqual(9, rom.mapper)
        self.assertEqual(128, rom.prg_rom_size)
        self.assertEqual(128, rom.chr_rom_size)
        self.assertFalse(rom.has_trainer)
        self.assertFalse(rom.is_pal)
        self.assertEqual('horizontal', rom.mirroring)

        env = self.env(path, render_mode='rgb_array')
        state, info = env.reset(seed=37)
        self.assert_valid_frame(state)
        self.assertIsInstance(info, dict)
        self.step_and_capture(env, DETERMINISTIC_ACTIONS)

        render = env.render()
        self.assertIs(render, env.screen)
        self.assert_valid_frame(render)

        env._backup()
        expected = self.step_and_capture(env, CONTINUATION_ACTIONS)
        env._restore()
        actual = self.step_and_capture(env, CONTINUATION_ACTIONS)

        for expected_output, actual_output in zip(expected, actual):
            self.assertTrue(np.array_equal(
                expected_output[0],
                actual_output[0],
            ))
            self.assertEqual(expected_output[1:], actual_output[1:])
