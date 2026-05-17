"""Mapper 003 / CNROM characterization tests."""

import numpy as np

from nes_py.nes_env import SCREEN_SHAPE_24_BIT

from nes_py.tests.mapper_fixtures import chr_bank_marker
from nes_py.tests.mapper_fixtures import prg_bank_marker
from nes_py.tests.mappers.common import MapperTestCase


class ShouldCharacterizeMapper003CNROM(MapperTestCase):
    """Characterize CNROM fixed PRG and switchable CHR ROM banking."""

    def test_fixed_prg_mapping_and_switchable_chr_rom_banks(self):
        path = self.synthetic_rom(
            'cnrom.nes',
            mapper=3,
            prg_banks=2,
            chr_banks=4,
        )
        env = self.env(path)

        self.assertEqual(prg_bank_marker(0), env._read_prg(0x9000))
        self.assertEqual(prg_bank_marker(1), env._read_prg(0xd000))
        self.assertEqual(chr_bank_marker(0), env._read_chr(0x0100))

        env._write_prg(0x8000, 0x02)
        self.assertEqual(chr_bank_marker(2), env._read_chr(0x0100))
        env._write_prg(0x8000, 0x03)
        self.assertEqual(chr_bank_marker(3), env._read_chr(0x0100))

    def test_screen_stays_valid_and_stable_across_chr_bank_changes(self):
        path = self.synthetic_rom(
            'cnrom-screen.nes',
            mapper=3,
            prg_banks=2,
            chr_banks=4,
        )
        env = self.env(path)

        env.reset()
        env._write_prg(0x8000, 0)
        env.step(0)
        before = env.render('rgb_array').copy()
        for bank in range(4):
            env._write_prg(0x8000, bank)
            env.step(0)
            self.assertEqual(SCREEN_SHAPE_24_BIT, env.screen.shape)

        env._write_prg(0x8000, 0)
        env.step(0)
        after = env.render('rgb_array')
        self.assertTrue(np.array_equal(before, after))

    def test_chr_bank_select_is_masked_to_available_chr_banks(self):
        path = self.synthetic_rom(
            'cnrom-masked-chr.nes',
            mapper=3,
            prg_banks=2,
            chr_banks=2,
        )
        env = self.env(path)

        env._write_prg(0x8000, 0x03)

        self.assertEqual(chr_bank_marker(1), env._read_chr(0x0100))
