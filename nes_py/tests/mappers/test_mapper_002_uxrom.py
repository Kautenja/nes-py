"""Mapper 002 / UxROM public environment tests."""

from nes_py._rom import ROM

from nes_py.tests.mappers.common import MapperTestCase


class ShouldLoadMapper002UxROM(MapperTestCase):
    """Exercise UxROM through the package API."""

    def test_chr_ram_fixture_constructs_steps_and_backup_restores(self):
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
        self.assertEqual('vertical', rom.mirroring)
        self.assert_env_smoke(path)
