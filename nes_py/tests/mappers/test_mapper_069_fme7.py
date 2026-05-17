"""Mapper 069 / Sunsoft FME-7 public environment tests.

The representative-title integration test uses only a local, legally supplied
ROM fixture at ``nes_py/tests/games/batman-return-of-the-joker.nes``. It never
fetches or downloads ROMs from the network.
"""

import os

from nes_py._rom import ROM

from nes_py.tests.mappers.common import MapperTestCase
from nes_py.tests.rom_file_abs_path import rom_file_abs_path


REPRESENTATIVE_TITLE = 'Batman - Return of the Joker (USA)'
REPRESENTATIVE_ROM = rom_file_abs_path('batman-return-of-the-joker.nes')
DETERMINISTIC_ACTIONS = (0, 1, 2, 4, 8, 16, 32, 64)
CONTINUATION_ACTIONS = (255, 128, 64, 32, 16, 8, 4, 2)


class ShouldLoadMapper069FME7(MapperTestCase):
    """Exercise FME-7 through the package API."""

    def test_synthetic_fme7_fixture_constructs_steps_and_renders(self):
        path = self.synthetic_rom(
            'fme7.nes',
            mapper=69,
            prg_banks=4,
            chr_banks=1,
        )

        rom = ROM(path)
        self.assertEqual(69, rom.mapper)
        self.assertEqual(64, rom.prg_rom_size)
        self.assertEqual(8, rom.chr_rom_size)
        self.assert_public_env_workflow(path)


class ShouldExerciseRepresentativeMapper069Title(MapperTestCase):
    """Exercise the representative FME-7 title through the package API."""

    def require_representative_rom(self):
        """Return the local representative ROM path or skip narrowly."""
        if not os.path.exists(REPRESENTATIVE_ROM):
            message = (
                'place a legally owned {} dump at {} to run this '
                'representative mapper 069 integration test; the test never '
                'fetches or downloads ROMs'
            )
            self.skipTest(message.format(
                REPRESENTATIVE_TITLE,
                REPRESENTATIVE_ROM,
            ))
        return REPRESENTATIVE_ROM

    def test_local_batman_return_of_the_joker_fixture_exercises_env(self):
        path = self.require_representative_rom()
        rom = ROM(path)
        self.assertEqual(69, rom.mapper)
        self.assertGreater(rom.prg_rom_size, 0)

        env = self.env(path, render_mode='rgb_array')
        state, info = env.reset(seed=69)
        self.assert_valid_frame(state)
        self.assertIsInstance(info, dict)
        self.step_and_capture(env, DETERMINISTIC_ACTIONS)

        render = env.render()
        self.assertIs(render, env.screen)
        self.assert_valid_frame(render)
        self.assert_backup_restore_workflow(env, CONTINUATION_ACTIONS)
