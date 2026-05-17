"""Mapper 000 / NROM characterization tests."""

from nes_py.nes_env import SCREEN_SHAPE_24_BIT

from nes_py.tests.mapper_fixtures import chr_bank_marker
from nes_py.tests.mapper_fixtures import prg_bank_marker
from nes_py.tests.mappers.common import MapperTestCase


class ShouldCharacterizeMapper000NROM(MapperTestCase):
    """Characterize NROM fixed PRG mapping and CHR ROM behavior."""

    def test_fixed_32k_prg_mapping_and_chr_rom_reads(self):
        path = self.synthetic_rom(
            'nrom-32k.nes',
            mapper=0,
            prg_banks=2,
            chr_banks=1,
            mirroring='vertical',
        )
        env = self.env(path)

        self.assertEqual(prg_bank_marker(0), env._read_prg(0x9000))
        self.assertEqual(prg_bank_marker(1), env._read_prg(0xd000))
        self.assertEqual(chr_bank_marker(0), env._read_chr(0x0100))

    def test_16k_prg_rom_is_mirrored(self):
        path = self.synthetic_rom(
            'nrom-16k.nes',
            mapper=0,
            prg_banks=1,
            chr_banks=1,
        )
        env = self.env(path)

        self.assertEqual(prg_bank_marker(0), env._read_prg(0x9000))
        self.assertEqual(prg_bank_marker(0), env._read_prg(0xd000))

    def test_reset_step_and_render_smoke(self):
        path = self.synthetic_rom(
            'nrom-smoke.nes',
            mapper=0,
            prg_banks=1,
            chr_banks=1,
        )
        env = self.env(path)

        state = env.reset()
        self.assertEqual(SCREEN_SHAPE_24_BIT, state.shape)
        state, reward, done, info = env.step(0)
        self.assertEqual(SCREEN_SHAPE_24_BIT, state.shape)
        self.assertIsInstance(reward, float)
        self.assertIsInstance(done, bool)
        self.assertIsInstance(info, dict)
        self.assertEqual(SCREEN_SHAPE_24_BIT, env.render('rgb_array').shape)

    def test_prg_ram_is_mapper_state_across_backup_restore(self):
        path = self.synthetic_rom(
            'nrom-prg-ram.nes',
            mapper=0,
            prg_banks=1,
            chr_banks=1,
        )
        env = self.env(path)

        env._write_prg(0x6000, 0x2a)
        env._backup()
        env._write_prg(0x6000, 0x7f)

        self.assertEqual(0x7f, env._read_prg(0x6000))
        env._restore()
        self.assertEqual(0x2a, env._read_prg(0x6000))
