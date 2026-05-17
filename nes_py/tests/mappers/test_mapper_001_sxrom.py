"""Mapper 001 / MMC1 / SxROM characterization tests."""

from nes_py.tests.mapper_fixtures import chr_page_marker
from nes_py.tests.mapper_fixtures import prg_bank_marker
from nes_py.tests.mappers.common import HORIZONTAL
from nes_py.tests.mappers.common import MapperTestCase
from nes_py.tests.mappers.common import ONE_SCREEN_HIGHER
from nes_py.tests.mappers.common import ONE_SCREEN_LOWER
from nes_py.tests.mappers.common import VERTICAL
from nes_py.tests.mappers.common import _write_mmc1_register


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
