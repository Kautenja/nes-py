"""Mapper 003 / CNROM public environment tests.

The representative-title integration test uses only a local, legally supplied
ROM fixture at ``nes_py/tests/games/adventure-island.nes``. It never fetches
or downloads ROMs from the network.
"""

import os

from nes_py._rom import ROM

from nes_py.tests.mappers.common import MapperTestCase
from nes_py.tests.rom_file_abs_path import rom_file_abs_path


REPRESENTATIVE_TITLE = 'Adventure Island (USA)'
REPRESENTATIVE_ROM = rom_file_abs_path('adventure-island.nes')
DETERMINISTIC_ACTIONS = (0, 1, 2, 4, 8, 16, 32, 64)
CONTINUATION_ACTIONS = (255, 128, 64, 32, 16, 8, 4, 2)


class ShouldLoadMapper003CNROM(MapperTestCase):
    """Exercise CNROM through the package API."""

    def test_chr_rom_fixture_constructs_steps_and_renders(self):
        path = self.synthetic_rom(
            'cnrom.nes',
            mapper=3,
            prg_banks=2,
            chr_banks=4,
        )

        rom = ROM(path)
        self.assertEqual(3, rom.mapper)
        self.assertEqual(32, rom.prg_rom_size)
        self.assertEqual(32, rom.chr_rom_size)
        self.assert_public_env_workflow(path)


class ShouldExerciseRepresentativeMapper003Title(MapperTestCase):
    """Exercise the representative CNROM title through the package API."""

    def require_representative_rom(self):
        """Return the local representative ROM path or skip narrowly."""
        if not os.path.exists(REPRESENTATIVE_ROM):
            message = (
                'place a legally owned {} dump at {} to run this '
                'representative mapper 003 integration test; the test never '
                'fetches or downloads ROMs'
            )
            self.skipTest(message.format(
                REPRESENTATIVE_TITLE,
                REPRESENTATIVE_ROM,
            ))
        return REPRESENTATIVE_ROM

    def test_local_adventure_island_fixture_exercises_env_and_render(self):
        path = self.require_representative_rom()
        rom = ROM(path)
        self.assertEqual(3, rom.mapper)
        self.assertEqual(32, rom.prg_rom_size)
        self.assertEqual(32, rom.chr_rom_size)
        self.assertFalse(rom.has_battery_backed_ram)
        self.assertEqual('vertical', rom.mirroring)

        env = self.env(path, render_mode='rgb_array')
        state, info = env.reset(seed=17)
        self.assert_valid_frame(state)
        self.assertIsInstance(info, dict)
        self.step_and_capture(env, DETERMINISTIC_ACTIONS)

        render = env.render()
        self.assertIs(render, env.screen)
        self.assert_valid_frame(render)
        self.assert_backup_restore_workflow(env, CONTINUATION_ACTIONS)
