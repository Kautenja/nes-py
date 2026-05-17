"""Mapper 000 / NROM public environment tests."""

from nes_py._rom import ROM

from nes_py.tests.mappers.common import MapperTestCase


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
