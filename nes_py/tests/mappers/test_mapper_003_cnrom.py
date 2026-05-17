"""Mapper 003 / CNROM public environment tests."""

from nes_py._rom import ROM

from nes_py.tests.mappers.common import MapperTestCase


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
