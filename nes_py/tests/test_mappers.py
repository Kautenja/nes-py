"""Characterization tests for currently supported native mappers."""
import tempfile
from unittest import TestCase

import numpy as np

from nes_py._rom import ROM
from nes_py.nes_env import NESEnv
from nes_py.nes_env import SCREEN_SHAPE_24_BIT
from nes_py.nes_env import _is_mapper_supported
from nes_py.nes_env import _native_mapper_bank_helper_smoke_results
from nes_py.nes_env import _native_mapper_hook_smoke_results
from .mapper_fixtures import chr_bank_marker
from .mapper_fixtures import chr_page_marker
from .mapper_fixtures import prg_bank_marker
from .mapper_fixtures import synthetic_rom_path


HORIZONTAL = 0
VERTICAL = 1
ONE_SCREEN_LOWER = 9
ONE_SCREEN_HIGHER = 10


def _write_mmc1_register(env, address, value):
    """Write an MMC1/SxROM register value through its serial load register."""
    for bit in range(5):
        env._write_prg(address, (value >> bit) & 0x01)


class MapperTestCase(TestCase):
    """Base test case that owns synthetic ROM temporary files."""

    def setUp(self):
        """Create a temporary directory for synthetic ROMs."""
        self.tmpdir = tempfile.TemporaryDirectory()

    def tearDown(self):
        """Remove temporary ROM files."""
        self.tmpdir.cleanup()

    def synthetic_rom(self, *args, **kwargs):
        """Create a synthetic ROM in this test's temporary directory."""
        return synthetic_rom_path(self.tmpdir.name, *args, **kwargs)

    def env(self, path):
        """Create an environment and close it during test cleanup."""
        env = NESEnv(path)
        self.addCleanup(self._close_env, env)
        return env

    def _close_env(self, env):
        """Close an environment if it is still open."""
        if env._env is not None:
            env.close()


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


class ShouldExposeMapperLifecycleAndTimingHooks(MapperTestCase):
    """Exercise native fake mappers that use the new extension points."""

    def test_native_mapper_hook_smoke_tests(self):
        results = _native_mapper_hook_smoke_results()

        self.assertEqual({
            'irq',
            'cpu_cycle',
            'ppu',
            'expansion',
            'prg_ram',
            'nametable',
        }, set(results))
        self.assertTrue(all(results.values()), results)

    def test_native_mapper_bank_helper_smoke_tests(self):
        results = _native_mapper_bank_helper_smoke_results()

        self.assertEqual({
            'prg_8k',
            'prg_16k',
            'prg_32k',
            'chr_1k',
            'chr_2k',
            'chr_4k',
            'chr_8k',
            'masks_and_bus_conflicts',
        }, set(results))
        self.assertTrue(all(results.values()), results)


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


class ShouldCharacterizeMapper001SxROM(MapperTestCase):
    """Characterize SxROM/MMC1 serial writes and stateful mapping."""

    def test_serial_register_writes_prg_switching_and_mirroring(self):
        path = self.synthetic_rom(
            'sxrom-prg.nes',
            mapper=1,
            prg_banks=4,
            chr_banks=0,
        )
        env = self.env(path)

        self.assertEqual(prg_bank_marker(0), env._read_prg(0x9000))
        self.assertEqual(prg_bank_marker(3), env._read_prg(0xd000))

        _write_mmc1_register(env, 0x8000, 0x0e)
        self.assertEqual(VERTICAL, env._name_table_mirroring())
        _write_mmc1_register(env, 0xe000, 0x02)
        self.assertEqual(prg_bank_marker(2), env._read_prg(0x9000))
        self.assertEqual(prg_bank_marker(3), env._read_prg(0xd000))

        _write_mmc1_register(env, 0x8000, 0x0f)
        self.assertEqual(HORIZONTAL, env._name_table_mirroring())

    def test_serial_reset_discards_partial_writes_and_sets_fixed_final_mode(self):
        path = self.synthetic_rom(
            'sxrom-serial-reset.nes',
            mapper=1,
            prg_banks=4,
            chr_banks=0,
        )
        env = self.env(path)

        _write_mmc1_register(env, 0x8000, 0x08)
        _write_mmc1_register(env, 0xe000, 0x02)
        self.assertEqual(prg_bank_marker(0), env._read_prg(0x9000))
        self.assertEqual(prg_bank_marker(2), env._read_prg(0xd000))

        env._write_prg(0xe000, 0x01)
        env._write_prg(0xe000, 0x01)
        env._write_prg(0x8000, 0x80)
        _write_mmc1_register(env, 0xe000, 0x01)

        self.assertEqual(prg_bank_marker(1), env._read_prg(0x9000))
        self.assertEqual(prg_bank_marker(3), env._read_prg(0xd000))

    def test_control_register_all_mirroring_modes(self):
        path = self.synthetic_rom(
            'sxrom-mirroring.nes',
            mapper=1,
            prg_banks=4,
            chr_banks=0,
        )
        env = self.env(path)

        cases = (
            (0x0c, ONE_SCREEN_LOWER),
            (0x0d, ONE_SCREEN_HIGHER),
            (0x0e, VERTICAL),
            (0x0f, HORIZONTAL),
        )
        for value, expected in cases:
            with self.subTest(control=value):
                _write_mmc1_register(env, 0x8000, value)
                self.assertEqual(expected, env._name_table_mirroring())

    def test_prg_bank_modes_mask_selection_safely(self):
        path = self.synthetic_rom(
            'sxrom-prg-modes.nes',
            mapper=1,
            prg_banks=4,
            chr_banks=0,
        )
        env = self.env(path)

        _write_mmc1_register(env, 0x8000, 0x00)
        _write_mmc1_register(env, 0xe000, 0x03)
        self.assertEqual(prg_bank_marker(2), env._read_prg(0x9000))
        self.assertEqual(prg_bank_marker(3), env._read_prg(0xd000))

        _write_mmc1_register(env, 0x8000, 0x08)
        _write_mmc1_register(env, 0xe000, 0x06)
        self.assertEqual(prg_bank_marker(0), env._read_prg(0x9000))
        self.assertEqual(prg_bank_marker(2), env._read_prg(0xd000))

        _write_mmc1_register(env, 0x8000, 0x0c)
        _write_mmc1_register(env, 0xe000, 0x06)
        self.assertEqual(prg_bank_marker(2), env._read_prg(0x9000))
        self.assertEqual(prg_bank_marker(3), env._read_prg(0xd000))

    def test_chr_rom_4k_and_8k_banking_low_bit_masking(self):
        path = self.synthetic_rom(
            'sxrom-chr-banks.nes',
            mapper=1,
            prg_banks=4,
            chr_banks=4,
            chr_4k_markers=True,
        )
        env = self.env(path)

        _write_mmc1_register(env, 0xa000, 0x03)
        self.assertEqual(chr_page_marker(2), env._read_chr(0x0000))
        self.assertEqual(chr_page_marker(3), env._read_chr(0x1000))

        _write_mmc1_register(env, 0x8000, 0x1e)
        _write_mmc1_register(env, 0xa000, 0x05)
        _write_mmc1_register(env, 0xc000, 0x06)
        self.assertEqual(chr_page_marker(5), env._read_chr(0x0000))
        self.assertEqual(chr_page_marker(6), env._read_chr(0x1000))

    def test_prg_ram_enable_and_protect_bits(self):
        path = self.synthetic_rom(
            'sxrom-prg-ram-protect.nes',
            mapper=1,
            prg_banks=4,
            chr_banks=0,
        )
        env = self.env(path)

        env._write_prg(0x6000, 0x11)
        _write_mmc1_register(env, 0xe000, 0x10)
        env._write_prg(0x6000, 0x22)
        self.assertEqual(0x11, env._read_prg(0x6000))

        _write_mmc1_register(env, 0xe000, 0x00)
        env._write_prg(0x6000, 0x33)
        self.assertEqual(0x33, env._read_prg(0x6000))

    def test_chr_ram_persistence_and_backup_restore_mapper_state(self):
        path = self.synthetic_rom(
            'sxrom-backup.nes',
            mapper=1,
            prg_banks=4,
            chr_banks=0,
        )
        env = self.env(path)

        _write_mmc1_register(env, 0x8000, 0x0f)
        _write_mmc1_register(env, 0xe000, 0x01)
        env._write_chr(0x0123, 0x5a)
        env._backup()

        _write_mmc1_register(env, 0x8000, 0x0e)
        _write_mmc1_register(env, 0xe000, 0x02)
        env._write_chr(0x0123, 0xa5)
        self.assertEqual(prg_bank_marker(2), env._read_prg(0x9000))
        self.assertEqual(0xa5, env._read_chr(0x0123))
        self.assertEqual(VERTICAL, env._name_table_mirroring())

        env._restore()
        self.assertEqual(prg_bank_marker(1), env._read_prg(0x9000))
        self.assertEqual(prg_bank_marker(3), env._read_prg(0xd000))
        self.assertEqual(0x5a, env._read_chr(0x0123))
        self.assertEqual(HORIZONTAL, env._name_table_mirroring())


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
