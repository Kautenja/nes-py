"""Mapper support registry and synthetic header metadata tests."""

from nes_py._rom import ROM
from nes_py.nes_env import _is_mapper_supported

from nes_py.tests.mappers.common import HORIZONTAL
from nes_py.tests.mappers.common import MapperTestCase
from nes_py.tests.mappers.common import VERTICAL


class ShouldIdentifySupportedMapperFixtures(MapperTestCase):
    """Identify the currently supported native mapper families."""

    def test_mapper_headers_and_native_metadata(self):
        cases = (
            ('nrom.nes', 0, 2, 1, 'vertical', False, VERTICAL),
            ('sxrom.nes', 1, 4, 0, 'horizontal', True, HORIZONTAL),
            ('uxrom.nes', 2, 4, 0, 'vertical', True, VERTICAL),
            ('cnrom.nes', 3, 2, 4, 'horizontal', False, HORIZONTAL),
        )

        for (
            name,
            mapper,
            prg_banks,
            chr_banks,
            mirroring,
            has_chr_ram,
            native_mirroring,
        ) in cases:
            with self.subTest(mapper=mapper):
                path = self.synthetic_rom(
                    name,
                    mapper=mapper,
                    prg_banks=prg_banks,
                    chr_banks=chr_banks,
                    mirroring=mirroring,
                )
                rom = ROM(path)
                env = self.env(path)

                self.assertEqual(mapper, rom.mapper)
                self.assertEqual(mapper, env._mapper_number())
                self.assertEqual(prg_banks * 16, rom.prg_rom_size)
                self.assertEqual(chr_banks * 8, rom.chr_rom_size)
                self.assertEqual(prg_banks * 0x4000, env._prg_rom_size())
                self.assertEqual(chr_banks * 0x2000, env._chr_rom_size())
                self.assertEqual(has_chr_ram, env._has_chr_ram())
                self.assertEqual(native_mirroring, env._name_table_mirroring())

    def test_mapper_factory_supports_registered_mapper_ids(self):
        for mapper in (0, 1, 2, 3):
            with self.subTest(mapper=mapper):
                self.assertTrue(_is_mapper_supported(mapper))

        self.assertFalse(_is_mapper_supported(4))
