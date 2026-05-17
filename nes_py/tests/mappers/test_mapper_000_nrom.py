"""Mapper 000 / NROM public environment tests.

The representative-title integration test uses only a local, legally supplied
ROM fixture at ``nes_py/tests/games/super-mario-bros-1.nes``. It never fetches
ROMs from the network.
"""

import os

from nes_py._rom import ROM

from nes_py.tests.mappers.common import MapperTestCase
from nes_py.tests.rom_file_abs_path import rom_file_abs_path


REPRESENTATIVE_TITLE = 'Super Mario Bros. (USA)'
REPRESENTATIVE_ROM = rom_file_abs_path('super-mario-bros-1.nes')
DETERMINISTIC_ACTIONS = (0, 1, 2, 4, 8, 16, 32, 64)
CONTINUATION_ACTIONS = (255, 128, 64, 32, 16, 8, 4, 2)


class ShouldLoadMapper000NROM(MapperTestCase):
    """Exercise NROM through the package API."""

    def test_32k_chr_rom_fixture_constructs_and_steps(self):
        path = self.synthetic_rom(
            'nrom-32k.nes',
            mapper=0,
            prg_banks=2,
            chr_banks=1,
            mirroring='vertical',
        )

        rom = ROM(path)
        self.assertEqual(0, rom.mapper)
        self.assertEqual(32, rom.prg_rom_size)
        self.assertEqual(8, rom.chr_rom_size)
        self.assertEqual('vertical', rom.mirroring)
        self.assert_public_env_workflow(path)

    def test_16k_prg_rom_fixture_constructs_and_steps(self):
        path = self.synthetic_rom(
            'nrom-16k.nes',
            mapper=0,
            prg_banks=1,
            chr_banks=1,
        )

        rom = ROM(path)
        self.assertEqual(0, rom.mapper)
        self.assertEqual(16, rom.prg_rom_size)
        self.assert_public_env_workflow(path)


class ShouldExerciseRepresentativeMapper000Title(MapperTestCase):
    """Exercise the representative NROM title through the package API."""

    def require_representative_rom(self):
        """Return the local representative ROM path or skip narrowly."""
        if not os.path.exists(REPRESENTATIVE_ROM):
            message = (
                'place a legally owned {} dump at {} to run this '
                'representative mapper 000 integration test'
            )
            self.skipTest(message.format(
                REPRESENTATIVE_TITLE,
                REPRESENTATIVE_ROM,
            ))
        return REPRESENTATIVE_ROM

    def test_local_super_mario_bros_fixture_exercises_env_and_render(self):
        path = self.require_representative_rom()
        rom = ROM(path)
        self.assertEqual(0, rom.mapper)
        self.assertEqual(32, rom.prg_rom_size)
        self.assertEqual(8, rom.chr_rom_size)
        self.assertEqual(8, rom.prg_ram_size)
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
