"""Mapper 007 / AxROM public environment tests.

The representative-title integration test uses only a local, legally supplied
ROM fixture at ``nes_py/tests/games/battletoads.nes``. It never fetches or
downloads ROMs from the network.
"""

import os

from nes_py._rom import ROM

from nes_py.tests.mappers.common import MapperTestCase
from nes_py.tests.rom_file_abs_path import rom_file_abs_path


REPRESENTATIVE_TITLE = 'Battletoads (USA)'
REPRESENTATIVE_ROM = rom_file_abs_path('battletoads.nes')
DETERMINISTIC_ACTIONS = (0, 1, 2, 4, 8, 16, 32, 64)
CONTINUATION_ACTIONS = (255, 128, 64, 32, 16, 8, 4, 2)


class ShouldLoadMapper007AxROM(MapperTestCase):
    """Exercise AxROM through the package API."""

    def test_chr_ram_fixture_constructs_and_steps(self):
        path = self.synthetic_rom(
            'axrom.nes',
            mapper=7,
            prg_banks=2,
            chr_banks=0,
        )

        rom = ROM(path)
        self.assertEqual(7, rom.mapper)
        self.assertEqual(32, rom.prg_rom_size)
        self.assertEqual(0, rom.chr_rom_size)
        self.assertEqual(8 * 2**10, rom.chr_ram_byte_size)
        self.assert_public_env_workflow(path)


class ShouldExerciseRepresentativeMapper007Title(MapperTestCase):
    """Exercise the representative AxROM title through the package API."""

    def require_representative_rom(self):
        """Return the local representative ROM path or skip narrowly."""
        if not os.path.exists(REPRESENTATIVE_ROM):
            message = (
                'place a legally owned {} dump at {} to run this '
                'representative mapper 007 integration test; the test suite '
                'does not fetch or download ROMs'
            )
            self.skipTest(message.format(
                REPRESENTATIVE_TITLE,
                REPRESENTATIVE_ROM,
            ))
        return REPRESENTATIVE_ROM

    def test_local_battletoads_fixture_exercises_env_reset_step_render_and_close(self):
        path = self.require_representative_rom()
        rom = ROM(path)
        self.assertEqual(7, rom.mapper)
        self.assertEqual(2, rom.submapper)
        self.assertEqual(256, rom.prg_rom_size)
        self.assertEqual(0, rom.chr_rom_size)
        self.assertEqual(8 * 2**10, rom.chr_ram_byte_size)
        self.assertFalse(rom.has_trainer)
        self.assertTrue(rom.is_nes2)
        self.assertFalse(rom.has_battery_backed_ram)

        env = self.env(path, render_mode='rgb_array')
        state, info = env.reset(seed=7)
        self.assert_valid_frame(state)
        self.assertIsInstance(info, dict)
        self.step_and_capture(env, DETERMINISTIC_ACTIONS)

        render = env.render()
        self.assertIs(render, env.screen)
        self.assert_valid_frame(render)
        self.assert_backup_restore_workflow(env, CONTINUATION_ACTIONS)
