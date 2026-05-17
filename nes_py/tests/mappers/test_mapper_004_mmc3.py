"""Mapper 004 / MMC3 public environment tests.

The representative-title integration test uses only a local, legally supplied
ROM fixture at ``nes_py/tests/games/super-mario-bros-3.nes``. It never fetches
or downloads ROMs from the network.
"""

import os

from nes_py._rom import ROM

from nes_py.tests.mappers.common import MapperTestCase
from nes_py.tests.rom_file_abs_path import rom_file_abs_path


REPRESENTATIVE_TITLE = 'Super Mario Bros. 3 (USA)'
REPRESENTATIVE_ROM = rom_file_abs_path('super-mario-bros-3.nes')
DETERMINISTIC_ACTIONS = (0, 1, 2, 4, 8, 16, 32, 64)
CONTINUATION_ACTIONS = (255, 128, 64, 32, 16, 8, 4, 2)


class ShouldLoadMapper004MMC3(MapperTestCase):
    """Exercise MMC3 through the package API."""

    def test_chr_rom_fixture_constructs_steps_and_renders(self):
        path = self.synthetic_rom(
            'mmc3.nes',
            mapper=4,
            prg_banks=4,
            chr_banks=2,
        )

        rom = ROM(path)
        self.assertEqual(4, rom.mapper)
        self.assertEqual(64, rom.prg_rom_size)
        self.assertEqual(16, rom.chr_rom_size)
        self.assertEqual('horizontal', rom.mirroring)
        self.assert_public_env_workflow(path)


class ShouldExerciseRepresentativeMapper004Title(MapperTestCase):
    """Exercise the representative MMC3 title through the package API."""

    def require_representative_rom(self):
        """Return the local representative ROM path or skip narrowly."""
        if not os.path.exists(REPRESENTATIVE_ROM):
            message = (
                'place a legally owned {} dump at {} to run this '
                'representative mapper 004 integration test; the test never '
                'fetches or downloads ROMs'
            )
            self.skipTest(message.format(
                REPRESENTATIVE_TITLE,
                REPRESENTATIVE_ROM,
            ))
        return REPRESENTATIVE_ROM

    def test_local_super_mario_bros_3_fixture_exercises_env_and_render(self):
        path = self.require_representative_rom()
        rom = ROM(path)
        self.assertEqual(4, rom.mapper)
        self.assertEqual(256, rom.prg_rom_size)
        self.assertEqual(128, rom.chr_rom_size)
        self.assertEqual(8, rom.prg_ram_size)
        self.assertFalse(rom.has_battery_backed_ram)
        self.assertFalse(rom.has_trainer)
        self.assertFalse(rom.is_nes2)
        self.assertEqual('horizontal', rom.mirroring)

        env = self.env(path, render_mode='rgb_array')
        state, info = env.reset(seed=17)
        self.assert_valid_frame(state)
        self.assertIsInstance(info, dict)
        self.step_and_capture(env, DETERMINISTIC_ACTIONS)

        render = env.render()
        self.assertIs(render, env.screen)
        self.assert_valid_frame(render)
        self.assert_backup_restore_workflow(env, CONTINUATION_ACTIONS)
