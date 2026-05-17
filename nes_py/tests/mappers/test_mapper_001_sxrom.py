"""Mapper 001 / MMC1 / SxROM public environment tests."""

from nes_py._rom import ROM

from nes_py.tests.mappers.common import MapperTestCase


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
