"""Mapper 002 / UxROM public environment tests.

The representative-title integration test uses only a local, legally supplied
ROM fixture at ``nes_py/tests/games/mega-man.nes``. It never fetches or
downloads ROMs from the network.
"""

import os

from nes_py._rom import ROM

from nes_py.tests.mappers.common import MapperTestCase
from nes_py.tests.rom_file_abs_path import rom_file_abs_path


REPRESENTATIVE_TITLE = 'Mega Man (USA)'
REPRESENTATIVE_ROM = rom_file_abs_path('mega-man.nes')
DETERMINISTIC_ACTIONS = (0, 1, 2, 4, 8, 16, 32, 64)
CONTINUATION_ACTIONS = (255, 128, 64, 32, 16, 8, 4, 2)


class ShouldLoadMapper002UxROM(MapperTestCase):
    """Exercise UxROM through the package API."""

    def test_chr_ram_fixture_constructs_and_steps(self):
        path = self.synthetic_rom(
            'uxrom.nes',
            mapper=2,
            prg_banks=4,
            chr_banks=0,
            mirroring='vertical',
        )

        rom = ROM(path)
        self.assertEqual(2, rom.mapper)
        self.assertEqual(64, rom.prg_rom_size)
        self.assertEqual(0, rom.chr_rom_size)
        self.assertEqual(8 * 2**10, rom.chr_ram_byte_size)
        self.assertEqual('vertical', rom.mirroring)
        self.assert_public_env_workflow(path)


class ShouldExerciseRepresentativeMapper002Title(MapperTestCase):
    """Exercise the representative UxROM title through the package API."""

    def require_representative_rom(self):
        """Return the local representative ROM path or skip narrowly."""
        if not os.path.exists(REPRESENTATIVE_ROM):
            message = (
                'place a legally owned {} dump at {} to run this '
                'representative mapper 002 integration test; the test suite '
                'does not fetch or download ROMs'
            )
            self.skipTest(message.format(
                REPRESENTATIVE_TITLE,
                REPRESENTATIVE_ROM,
            ))
        return REPRESENTATIVE_ROM

    def test_local_mega_man_fixture_exercises_env_reset_step_render_and_close(self):
        path = self.require_representative_rom()
        rom = ROM(path)
        self.assertEqual(2, rom.mapper)
        self.assertEqual(128, rom.prg_rom_size)
        self.assertEqual(0, rom.chr_rom_size)
        self.assertEqual(8 * 2**10, rom.chr_ram_byte_size)
        self.assertFalse(rom.has_trainer)
        self.assertFalse(rom.is_nes2)
        self.assertEqual('vertical', rom.mirroring)

        env = self.env(path, render_mode='rgb_array')
        state, info = env.reset(seed=31)
        self.assert_valid_frame(state)
        self.assertIsInstance(info, dict)
        self.step_and_capture(env, DETERMINISTIC_ACTIONS)

        render = env.render()
        self.assertIs(render, env.screen)
        self.assert_valid_frame(render)
        self.assert_backup_restore_workflow(env, CONTINUATION_ACTIONS)
