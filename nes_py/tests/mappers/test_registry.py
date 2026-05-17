"""Synthetic mapper construction and rejection tests."""

from nes_py._rom import ROM
from nes_py.nes_env import NESEnv

from nes_py.tests.mappers.common import MapperTestCase


class ShouldLoadSupportedMapperFixtures(MapperTestCase):
    """Exercise currently supported mapper fixtures through NESEnv."""

    def test_mapper_headers_construct_and_step(self):
        cases = (
            ('nrom.nes', 0, 2, 1, 'vertical'),
            ('sxrom.nes', 1, 4, 0, 'horizontal'),
            ('uxrom.nes', 2, 4, 0, 'vertical'),
            ('cnrom.nes', 3, 2, 4, 'horizontal'),
            ('fme7.nes', 69, 4, 1, 'horizontal'),
        )

        for (
            name,
            mapper,
            prg_banks,
            chr_banks,
            mirroring,
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
                env = self.env(path, render_mode='rgb_array')

                self.assertEqual(mapper, rom.mapper)
                self.assertEqual(prg_banks * 16, rom.prg_rom_size)
                self.assertEqual(chr_banks * 8, rom.chr_rom_size)
                self.assertEqual(mirroring, rom.mirroring)
                state, reset_info = env.reset()
                self.assertEqual((240, 256, 3), state.shape)
                self.assertIsInstance(reset_info, dict)
                state, reward, terminated, truncated, info = env.step(0)
                self.assertEqual((240, 256, 3), state.shape)
                self.assertIsInstance(reward, float)
                self.assertIsInstance(terminated, bool)
                self.assertIsInstance(truncated, bool)
                self.assertFalse(truncated)
                self.assertIsInstance(info, dict)
                self.assertEqual((240, 256, 3), env.render().shape)

    def test_public_constructor_rejects_unsupported_mapper(self):
        path = self.synthetic_rom(
            'unsupported.nes',
            mapper=4,
            prg_banks=2,
            chr_banks=1,
        )

        with self.assertRaises(ValueError) as error:
            NESEnv(path)
        self.assertIn('unsupported mapper number 4', str(error.exception))
