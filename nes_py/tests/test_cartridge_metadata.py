"""Parser alignment tests for Python ROM and native Cartridge metadata."""
import tempfile
from pathlib import Path
from unittest import TestCase

from nes_py._rom import ROM
from nes_py.nes_env import NESEnv
from nes_py.nes_env import _native_cartridge_error
from nes_py.nes_env import _native_cartridge_metadata

from .mapper_fixtures import CHR_BANK_SIZE
from .mapper_fixtures import PRG_BANK_SIZE
from .mapper_fixtures import ines_header
from .mapper_fixtures import synthetic_rom_path
from .rom_file_abs_path import rom_file_abs_path


HORIZONTAL = 0
VERTICAL = 1
FOUR_SCREEN = 8


class CartridgeMetadataTestCase(TestCase):
    """Base test case that owns synthetic ROM temporary files."""

    def setUp(self):
        """Create a temporary directory for synthetic ROMs."""
        self.tmpdir = tempfile.TemporaryDirectory()

    def tearDown(self):
        """Remove temporary ROM files."""
        self.tmpdir.cleanup()

    def write_rom(self, name, data):
        """Write ROM bytes into this test's temporary directory."""
        path = Path(self.tmpdir.name) / name
        path.write_bytes(data)
        return str(path)

    def synthetic_rom(self, *args, **kwargs):
        """Create a synthetic ROM in this test's temporary directory."""
        return synthetic_rom_path(self.tmpdir.name, *args, **kwargs)

    def assert_metadata_agrees(self, path):
        """Assert Python ROM metadata matches native Cartridge metadata."""
        rom = ROM(path)
        native = _native_cartridge_metadata(path)
        mirroring = {
            'horizontal': HORIZONTAL,
            'vertical': VERTICAL,
            'four-screen': FOUR_SCREEN,
        }[rom.mirroring]

        self.assertEqual(rom.mapper, native['mapper'])
        self.assertEqual(rom.submapper, native['submapper'])
        self.assertEqual(rom.prg_rom_byte_size, native['prg_rom_byte_size'])
        self.assertEqual(rom.prg_rom_banks, native['prg_rom_banks'])
        self.assertEqual(rom.chr_rom_byte_size, native['chr_rom_byte_size'])
        self.assertEqual(rom.chr_rom_banks, native['chr_rom_banks'])
        self.assertEqual(rom.prg_ram_byte_size, native['prg_ram_byte_size'])
        self.assertEqual(
            rom.prg_battery_ram_byte_size,
            native['prg_battery_ram_byte_size']
        )
        self.assertEqual(rom.chr_ram_byte_size, native['chr_ram_byte_size'])
        self.assertEqual(
            rom.chr_battery_ram_byte_size,
            native['chr_battery_ram_byte_size']
        )
        self.assertEqual(rom.has_trainer, native['has_trainer'])
        self.assertEqual(rom.trainer_rom_start, native['trainer_rom_start'])
        self.assertEqual(rom.trainer_rom_stop, native['trainer_rom_stop'])
        self.assertEqual(
            rom.has_battery_backed_ram,
            native['has_battery_backed_ram']
        )
        self.assertEqual(mirroring, native['name_table_mirroring'])
        self.assertEqual(rom.has_vs_unisystem, native['has_vs_unisystem'])
        self.assertEqual(rom.has_play_choice_10, native['has_play_choice_10'])
        self.assertEqual(rom.is_pal, native['is_pal'])
        self.assertEqual(rom.is_nes2, native['is_nes2'])

    def assert_python_and_native_reject(self, data, fragment):
        """Assert Python and native parsers reject ROM data consistently."""
        path = self.write_rom('invalid.nes', data)

        with self.assertRaises(ValueError) as error:
            ROM(path)
        self.assertIn(fragment, str(error.exception))

        native_error = _native_cartridge_error(path)
        self.assertIsNotNone(native_error)
        self.assertIn(fragment, native_error)


class ShouldAlignCartridgeMetadata(CartridgeMetadataTestCase):
    """Check Python ROM and native Cartridge metadata agreement."""

    def test_existing_fixture_metadata_agrees(self):
        cases = (
            'super-mario-bros-1.nes',
            'the-legend-of-zelda.nes',
        )
        for name in cases:
            with self.subTest(name=name):
                self.assert_metadata_agrees(rom_file_abs_path(name))

    def test_ines_synthetic_metadata_agrees(self):
        path = self.synthetic_rom(
            'battery-chr-ram.nes',
            mapper=2,
            prg_banks=2,
            chr_banks=0,
            mirroring='vertical',
            battery=True,
            prg_ram_banks=2,
            vs_unisystem=True,
            play_choice_10=True,
        )

        self.assert_metadata_agrees(path)
        rom = ROM(path)
        self.assertEqual(16 * 2**10, rom.prg_ram_byte_size)
        self.assertEqual(16 * 2**10, rom.prg_battery_ram_byte_size)
        self.assertEqual(CHR_BANK_SIZE, rom.chr_ram_byte_size)

    def test_nes2_synthetic_metadata_agrees(self):
        path = self.synthetic_rom(
            'nes2-mapper.nes',
            mapper=0x123,
            submapper=0x07,
            prg_banks=1,
            chr_banks=0,
            nes2=True,
            prg_ram_shift=7,
            prg_battery_ram_shift=8,
            chr_ram_shift=7,
            chr_battery_ram_shift=6,
        )

        self.assert_metadata_agrees(path)
        rom = ROM(path)
        self.assertEqual(0x123, rom.mapper)
        self.assertEqual(0x07, rom.submapper)
        self.assertEqual(8 * 2**10, rom.prg_ram_byte_size)
        self.assertEqual(16 * 2**10, rom.prg_battery_ram_byte_size)
        self.assertEqual(8 * 2**10, rom.chr_ram_byte_size)
        self.assertEqual(4 * 2**10, rom.chr_battery_ram_byte_size)


class ShouldRejectMalformedCartridges(CartridgeMetadataTestCase):
    """Check deterministic Python and native rejection paths."""

    def test_invalid_magic_and_truncated_header_are_rejected(self):
        self.assert_python_and_native_reject(
            b'NOPE' + bytes(12),
            'ROM missing magic number in header.'
        )
        self.assert_python_and_native_reject(
            b'NES\x1a',
            'ROM header is truncated.'
        )

    def test_truncated_prg_and_chr_payloads_are_rejected(self):
        self.assert_python_and_native_reject(
            ines_header(0, 1, 0),
            'failed to read PRG-ROM on ROM.'
        )
        self.assert_python_and_native_reject(
            ines_header(0, 1, 1) + bytes(PRG_BANK_SIZE),
            'failed to read CHR-ROM on ROM.'
        )


class ShouldHandleUnsupportedHeaderFeatures(CartridgeMetadataTestCase):
    """Check trainer, PAL, and four-screen edge metadata."""

    def test_trainer_offsets_are_parsed_and_nes_env_names_the_rejection(self):
        path = self.synthetic_rom(
            'trainer.nes',
            mapper=0,
            prg_banks=1,
            chr_banks=1,
            trainer=True,
        )

        self.assert_metadata_agrees(path)
        rom = ROM(path)
        self.assertEqual(16, rom.trainer_rom_start)
        self.assertEqual(16 + 512, rom.trainer_rom_stop)

        native_error = _native_cartridge_error(path)
        self.assertIn('trainer is not supported', native_error)
        with self.assertRaises(ValueError) as error:
            NESEnv(path)
        self.assertIn('trainer is not supported', str(error.exception))

    def test_pal_metadata_is_parsed_and_nes_env_names_the_rejection(self):
        path = self.synthetic_rom(
            'pal.nes',
            mapper=0,
            prg_banks=1,
            chr_banks=1,
            pal=True,
        )

        self.assert_metadata_agrees(path)
        self.assertTrue(ROM(path).is_pal)
        native_error = _native_cartridge_error(path)
        self.assertIn('PAL is not supported', native_error)
        with self.assertRaises(ValueError) as error:
            NESEnv(path)
        self.assertIn('PAL is not supported', str(error.exception))

    def test_four_screen_mirroring_does_not_become_one_screen_lower(self):
        header = bytearray(ines_header(0, 1, 0, mirroring='four-screen'))
        header[6] |= 0x01
        data = bytes(header) + bytes(PRG_BANK_SIZE)
        path = self.write_rom('four-screen-vertical-bit.nes', data)

        self.assert_metadata_agrees(path)
        self.assertEqual('four-screen', ROM(path).mirroring)
        self.assertEqual(
            FOUR_SCREEN,
            _native_cartridge_metadata(path)['name_table_mirroring']
        )

        env = NESEnv(path)
        self.addCleanup(env.close)
        self.assertEqual(FOUR_SCREEN, env._name_table_mirroring())

    def test_default_ines_chr_ram_is_visible_to_native_mapper(self):
        path = self.synthetic_rom(
            'nrom-chr-ram.nes',
            mapper=0,
            prg_banks=1,
            chr_banks=0,
        )

        native = _native_cartridge_metadata(path)
        self.assertEqual(CHR_BANK_SIZE, native['chr_ram_byte_size'])

        env = NESEnv(path)
        self.addCleanup(env.close)
        self.assertTrue(env._has_chr_ram())
        env._write_chr(0x0123, 0x5a)
        self.assertEqual(0x5a, env._read_chr(0x0123))
