"""Application-level ROM metadata and cartridge rejection tests."""
import tempfile
from pathlib import Path
from unittest import TestCase

from nes_py._rom import ROM
from nes_py.nes_env import NESEnv

from nes_py.tests.mapper_fixtures import CHR_BANK_SIZE
from nes_py.tests.mapper_fixtures import PRG_BANK_SIZE
from nes_py.tests.mapper_fixtures import ines_header
from nes_py.tests.mapper_fixtures import synthetic_rom_path
from nes_py.tests.rom_file_abs_path import rom_file_abs_path


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

    def assert_rom_rejects(self, data, fragment):
        """Assert public ROM construction rejects malformed bytes."""
        path = self.write_rom('invalid.nes', data)

        with self.assertRaises(ValueError) as error:
            ROM(path)
        self.assertIn(fragment, str(error.exception))


class ShouldParseApplicationROMMetadata(CartridgeMetadataTestCase):
    """Check Python ROM metadata for disk and synthetic fixtures."""

    def test_existing_fixture_metadata_is_available_without_env_construction(self):
        cases = (
            ('super-mario-bros-1.nes', 0, 32, 8, 'vertical'),
            ('super-mario-bros-2.nes', 4, 128, 128, 'horizontal'),
            ('super-mario-bros-3.nes', 4, 256, 128, 'horizontal'),
            ('super-mario-bros-lost-levels.nes', 0, 32, 8, 'vertical'),
            ('the-legend-of-zelda.nes', 1, 128, 0, 'horizontal'),
            ('excitebike.nes', 0, 16, 8, 'vertical'),
        )

        for name, mapper, prg_size, chr_size, mirroring in cases:
            with self.subTest(name=name):
                rom = ROM(rom_file_abs_path(name))

                self.assertEqual(mapper, rom.mapper)
                self.assertEqual(prg_size, rom.prg_rom_size)
                self.assertEqual(chr_size, rom.chr_rom_size)
                self.assertEqual(mirroring, rom.mirroring)

    def test_ines_synthetic_metadata_is_available_to_public_rom_parser(self):
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

        rom = ROM(path)
        self.assertEqual(2, rom.mapper)
        self.assertEqual(2, rom.prg_rom_banks)
        self.assertEqual(0, rom.chr_rom_banks)
        self.assertEqual(16 * 2**10, rom.prg_ram_byte_size)
        self.assertEqual(16 * 2**10, rom.prg_battery_ram_byte_size)
        self.assertEqual(CHR_BANK_SIZE, rom.chr_ram_byte_size)
        self.assertTrue(rom.has_battery_backed_ram)
        self.assertTrue(rom.has_vs_unisystem)
        self.assertTrue(rom.has_play_choice_10)
        self.assertEqual('vertical', rom.mirroring)

    def test_nes2_synthetic_metadata_is_available_to_public_rom_parser(self):
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

        rom = ROM(path)
        self.assertTrue(rom.is_nes2)
        self.assertEqual(0x123, rom.mapper)
        self.assertEqual(0x07, rom.submapper)
        self.assertEqual(PRG_BANK_SIZE, rom.prg_rom_byte_size)
        self.assertEqual(8 * 2**10, rom.prg_ram_byte_size)
        self.assertEqual(16 * 2**10, rom.prg_battery_ram_byte_size)
        self.assertEqual(8 * 2**10, rom.chr_ram_byte_size)
        self.assertEqual(4 * 2**10, rom.chr_battery_ram_byte_size)


class ShouldRejectMalformedCartridges(CartridgeMetadataTestCase):
    """Check deterministic public rejection paths for malformed ROMs."""

    def test_invalid_magic_and_truncated_header_are_rejected(self):
        self.assert_rom_rejects(
            b'NOPE' + bytes(12),
            'ROM missing magic number in header.'
        )
        self.assert_rom_rejects(
            b'NES\x1a',
            'ROM header is truncated.'
        )

    def test_nonzero_ines_header_padding_is_rejected(self):
        header = bytearray(ines_header(0, 1, 0))
        header[12] = 0x01

        self.assert_rom_rejects(
            bytes(header) + bytes(PRG_BANK_SIZE),
            'ROM header zero fill bytes are not zero.'
        )

    def test_truncated_prg_and_chr_payloads_are_rejected(self):
        self.assert_rom_rejects(
            ines_header(0, 1, 0),
            'failed to read PRG-ROM on ROM.'
        )
        self.assert_rom_rejects(
            ines_header(0, 1, 1) + bytes(PRG_BANK_SIZE),
            'failed to read CHR-ROM on ROM.'
        )

    def test_rom_without_prg_banks_is_rejected_by_environment(self):
        path = self.write_rom('no-prg.nes', ines_header(0, 0, 0))

        with self.assertRaises(ValueError) as error:
            NESEnv(path)
        self.assertIn('ROM has no PRG-ROM banks', str(error.exception))


class ShouldHandleUnsupportedHeaderFeatures(CartridgeMetadataTestCase):
    """Check public construction behavior for unsupported cartridge features."""

    def test_trainer_offsets_are_parsed_and_nes_env_names_the_rejection(self):
        path = self.synthetic_rom(
            'trainer.nes',
            mapper=0,
            prg_banks=1,
            chr_banks=1,
            trainer=True,
        )

        rom = ROM(path)
        self.assertTrue(rom.has_trainer)
        self.assertEqual(16, rom.trainer_rom_start)
        self.assertEqual(16 + 512, rom.trainer_rom_stop)

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

        self.assertTrue(ROM(path).is_pal)
        with self.assertRaises(ValueError) as error:
            NESEnv(path)
        self.assertIn('PAL is not supported', str(error.exception))

    def test_unsupported_mapper_rejection_names_the_mapper(self):
        path = self.synthetic_rom(
            'unsupported.nes',
            mapper=0x123,
            prg_banks=1,
            chr_banks=1,
            nes2=True,
        )

        self.assertEqual(0x123, ROM(path).mapper)
        with self.assertRaises(ValueError) as error:
            NESEnv(path)
        self.assertIn('unsupported mapper number 291', str(error.exception))

    def test_four_screen_mirroring_prefers_ignore_bit(self):
        header = bytearray(ines_header(0, 1, 0, mirroring='four-screen'))
        header[6] |= 0x01
        data = bytes(header) + bytes(PRG_BANK_SIZE)
        path = self.write_rom('four-screen-vertical-bit.nes', data)

        rom = ROM(path)
        self.assertEqual('four-screen', rom.mirroring)

        env = NESEnv(path)
        try:
            state, info = env.reset()
            self.assertEqual((240, 256, 3), state.shape)
            self.assertIsInstance(info, dict)
            self.assertEqual((240, 256, 3), env.step(0)[0].shape)
        finally:
            env.close()

    def test_default_ines_chr_ram_rom_constructs_and_steps(self):
        path = self.synthetic_rom(
            'nrom-chr-ram.nes',
            mapper=0,
            prg_banks=1,
            chr_banks=0,
        )

        rom = ROM(path)
        self.assertEqual(CHR_BANK_SIZE, rom.chr_ram_byte_size)

        env = NESEnv(path)
        try:
            state, info = env.reset()
            self.assertEqual((240, 256, 3), state.shape)
            self.assertIsInstance(info, dict)
            self.assertEqual((240, 256, 3), env.step(0)[0].shape)
        finally:
            env.close()
