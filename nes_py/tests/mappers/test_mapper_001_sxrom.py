"""Mapper 001 / MMC1 / SxROM public environment tests.

The representative-title integration test uses only a local, legally supplied
ROM fixture at ``nes_py/tests/games/the-legend-of-zelda.nes``. It never
fetches ROMs from the network.
"""

import os

import numpy as np

from nes_py._rom import ROM
from nes_py.nes_env import SCREEN_SHAPE_24_BIT
from nes_py.tests.mappers.common import MapperTestCase
from nes_py.tests.rom_file_abs_path import rom_file_abs_path


REPRESENTATIVE_TITLE = 'The Legend of Zelda (USA)'
REPRESENTATIVE_ROM = rom_file_abs_path('the-legend-of-zelda.nes')
DETERMINISTIC_ACTIONS = (0, 1, 2, 4, 8, 16, 32, 64)
CONTINUATION_ACTIONS = (255, 128, 64, 32, 16, 8, 4, 2, 1, 0)


class ShouldLoadMapper001SxROM(MapperTestCase):
    """Exercise SxROM through the package API."""

    def test_chr_ram_fixture_constructs_and_steps(self):
        path = self.synthetic_rom(
            'sxrom-chr-ram.nes',
            mapper=1,
            prg_banks=4,
            chr_banks=0,
        )

        rom = ROM(path)
        self.assertEqual(1, rom.mapper)
        self.assertEqual(64, rom.prg_rom_size)
        self.assertEqual(0, rom.chr_rom_size)
        self.assertEqual(8 * 2**10, rom.chr_ram_byte_size)
        self.assert_public_env_workflow(path)

    def test_chr_rom_fixture_constructs_and_steps(self):
        path = self.synthetic_rom(
            'sxrom-chr-rom.nes',
            mapper=1,
            prg_banks=4,
            chr_banks=4,
            chr_4k_markers=True,
        )

        rom = ROM(path)
        self.assertEqual(1, rom.mapper)
        self.assertEqual(32, rom.chr_rom_size)
        self.assert_public_env_workflow(path)


class ShouldExerciseRepresentativeMapper001Title(MapperTestCase):
    """Exercise the representative MMC1 title through the package API."""

    def require_representative_rom(self):
        """Return the local representative ROM path or skip narrowly."""
        if not os.path.exists(REPRESENTATIVE_ROM):
            message = (
                'place a legally owned {} dump at {} to run this '
                'representative mapper 001 integration test'
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

    def test_local_zelda_fixture_exercises_env_render_and_backup_restore(self):
        path = self.require_representative_rom()
        rom = ROM(path)
        self.assertEqual(1, rom.mapper)
        self.assertEqual(128, rom.prg_rom_size)
        self.assertEqual(0, rom.chr_rom_size)
        self.assertEqual(8, rom.prg_ram_size)
        self.assertTrue(rom.has_battery_backed_ram)
        self.assertEqual('horizontal', rom.mirroring)

        env = self.env(path, render_mode='rgb_array')
        state, info = env.reset(seed=17)
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
