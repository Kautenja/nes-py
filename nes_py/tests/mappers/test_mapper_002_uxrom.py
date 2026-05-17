"""Mapper 002 / UxROM characterization tests."""

from nes_py.tests.mapper_fixtures import prg_bank_marker
from nes_py.tests.mappers.common import MapperTestCase


class ShouldCharacterizeMapper002UxROM(MapperTestCase):
    """Characterize UxROM bank switching and CHR RAM behavior."""

    def test_switchable_prg_bank_fixed_final_bank_and_chr_ram(self):
        path = self.synthetic_rom(
            'uxrom.nes',
            mapper=2,
            prg_banks=4,
            chr_banks=0,
        )
        env = self.env(path)

        self.assertEqual(prg_bank_marker(0), env._read_prg(0x9000))
        self.assertEqual(prg_bank_marker(3), env._read_prg(0xd000))

        env._write_prg(0x8000, 0x02)
        self.assertEqual(prg_bank_marker(2), env._read_prg(0x9000))
        self.assertEqual(prg_bank_marker(3), env._read_prg(0xd000))

        env._write_chr(0x0456, 0x3c)
        self.assertEqual(0x3c, env._read_chr(0x0456))

    def test_bank_select_writes_take_the_value_without_bus_conflicts(self):
        path = self.synthetic_rom(
            'uxrom-bus-conflict.nes',
            mapper=2,
            prg_banks=4,
            chr_banks=0,
        )
        env = self.env(path)

        env._write_prg(0x8000, 0x01)

        self.assertEqual(prg_bank_marker(1), env._read_prg(0x9000))

    def test_bank_select_is_masked_to_available_prg_banks(self):
        path = self.synthetic_rom(
            'uxrom-masked-bank.nes',
            mapper=2,
            prg_banks=4,
            chr_banks=0,
        )
        env = self.env(path)

        env._write_prg(0x8000, 0x05)

        self.assertEqual(prg_bank_marker(1), env._read_prg(0x9000))
