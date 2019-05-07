"""Test cases for the ROM.

Information about ROMs if found here:
http://tuxnes.sourceforge.net/nesmapper.txt

"""
from unittest import TestCase
from .rom_file_abs_path import rom_file_abs_path
from nes_py._rom import ROM


class ShouldNotCreateInstanceOfROMWithoutPath(TestCase):
    def test(self):
        self.assertRaises(TypeError, ROM)


class ShouldNotCreateInstanceOfROMWithInvaldPath(TestCase):
    def test(self):
        self.assertRaises(TypeError, lambda: ROM(5))
        self.assertRaises(ValueError, lambda: ROM('not a path'))


class ShouldNotCreateInstanceOfROMWithInvaldROMFile(TestCase):
    def test(self):
        empty = rom_file_abs_path('empty.nes')
        self.assertRaises(ValueError, lambda: ROM(empty))


#
# MARK: ROM Headers
#


class ShouldReadROMHeaderTestCase(object):
    """The general form of a test case to check the header of a ROM."""

    # the name of the ROM to test the header of
    rom_name = None

    # the amount of program memory in the ROM (KB)
    prg_rom_size = None

    # the amount of character map memory in the ROM (KB)
    chr_rom_size = None

    # the amount of program RAM in the ROM (KB)
    prg_ram_size = None

    # the number of the mapper this ROM uses
    mapper = None

    # whether to ignore the mirroring setting bit
    is_ignore_mirroring = None

    # whether the ROM contains a trainer block
    has_trainer = None

    # whether the cartridge of the ROM has a battery-backed RAM module
    has_battery_backed_ram = None

    # the mirroring mode used by the ROM
    is_vertical_mirroring = None

    # whether the ROM uses PlayChoice-10 (8KB of Hint Screen after CHR data)
    has_play_choice_10 = None

    # whether the ROM uses VS Unisystem
    has_vs_unisystem = None

    # the TV system the ROM is designed for
    is_pal = None

    # the address the trainer ROM starts at
    trainer_rom_start = None

    # the address the trainer ROM stops at
    trainer_rom_stop = None

    # the address the PRG ROM starts at
    prg_rom_start = None

    # the address the PRG ROM stops at
    prg_rom_stop = None

    # the address the CHR ROM starts at
    chr_rom_start = None

    # the address the CHR ROM stops at
    chr_rom_stop = None

    def setUp(self):
        """Perform setup before each test."""
        rom_path = rom_file_abs_path(self.rom_name)
        self.rom = ROM(rom_path)

    def test_header_length(self):
        """Check the length of the header."""
        self.assertEqual(16, len(self.rom.header))

    def test_prg_rom_size(self):
        """Check the PRG ROM size."""
        self.assertEqual(self.prg_rom_size, self.rom.prg_rom_size)

    def test_chr_rom_size(self):
        """Check the CHR ROM size."""
        self.assertEqual(self.chr_rom_size, self.rom.chr_rom_size)

    def test_prg_ram_size(self):
        """Check the PRG RAM size."""
        self.assertEqual(self.prg_ram_size, self.rom.prg_ram_size)

    def test_mapper(self):
        """Check the mapper number."""
        self.assertEqual(self.mapper, self.rom.mapper)

    def test_is_ignore_mirroring(self):
        """Check whether the ROM is ignoring the mirroring mode."""
        expected = self.is_ignore_mirroring
        actual = self.rom.is_ignore_mirroring
        self.assertEqual(expected, actual)

    def test_has_trainer(self):
        """Check whether the ROM has a trainer block or not."""
        self.assertEqual(self.has_trainer, self.rom.has_trainer)

    def test_has_battery_backed_ram(self):
        """Check whether the ROM has battery-backed RAM."""
        expected = self.has_battery_backed_ram
        actual = self.rom.has_battery_backed_ram
        self.assertEqual(expected, actual)

    def test_is_vertical_mirroring(self):
        """Check the mirroring mode of the ROM."""
        self.assertEqual(self.is_vertical_mirroring, self.rom.is_vertical_mirroring)

    def test_has_play_choice_10(self):
        """Check whether the ROM uses PlayChoice-10."""
        self.assertEqual(self.has_play_choice_10, self.rom.has_play_choice_10)

    def test_has_vs_unisystem(self):
        """Check whether the ROM uses a VS Unisystem."""
        self.assertEqual(self.has_vs_unisystem, self.rom.has_vs_unisystem)

    def test_is_pal(self):
        """Check which TV mode the ROM is designed for."""
        self.assertEqual(self.is_pal, self.rom.is_pal)

    def test_trainer_rom_start(self):
        """Check the starting address of trainer ROM."""
        self.assertEqual(self.trainer_rom_start, self.rom.trainer_rom_start)

    def test_trainer_rom_stop(self):
        """Check the stopping address of trainer ROM."""
        self.assertEqual(self.trainer_rom_stop, self.rom.trainer_rom_stop)

    def test_trainer_rom(self):
        """Check the trainer ROM."""
        size = self.trainer_rom_stop - self.trainer_rom_start
        self.assertEqual(size, len(self.rom.trainer_rom))

    def test_prg_rom_start(self):
        """Check the starting address of PRG ROM."""
        self.assertEqual(self.prg_rom_start, self.rom.prg_rom_start)

    def test_prg_rom_stop(self):
        """Check the stopping address of PRG ROM."""
        self.assertEqual(self.prg_rom_stop, self.rom.prg_rom_stop)

    def test_prg_rom(self):
        """Check the  PRG ROM."""
        size = (self.prg_rom_stop - self.prg_rom_start)
        self.assertEqual(size, len(self.rom.prg_rom))

    def test_chr_rom_start(self):
        """Check the starting address of CHR ROM."""
        self.assertEqual(self.chr_rom_start, self.rom.chr_rom_start)

    def test_chr_rom_stop(self):
        """Check the stopping address of CHR ROM."""
        self.assertEqual(self.chr_rom_stop, self.rom.chr_rom_stop)

    def test_chr_rom(self):
        """Check the CHR ROM."""
        size = (self.chr_rom_stop - self.chr_rom_start)
        self.assertEqual(size, len(self.rom.chr_rom))


class ShouldReadSuperMarioBros(ShouldReadROMHeaderTestCase, TestCase):
    """Check the Super Mario Bros 1 ROM."""

    rom_name = 'super-mario-bros-1.nes'
    prg_rom_size = 32
    chr_rom_size = 8
    prg_ram_size = 8
    mapper = 0
    is_ignore_mirroring = False
    has_trainer = False
    has_battery_backed_ram = False
    is_vertical_mirroring = True
    has_play_choice_10 = False
    has_vs_unisystem = False
    is_pal = False
    trainer_rom_start = 16
    trainer_rom_stop = 16
    prg_rom_start = 16
    prg_rom_stop = 16 + 32 * 2**10
    chr_rom_start = 16 + 32 * 2**10
    chr_rom_stop = (16 + 32 * 2**10) + (8 * 2**10)


class ShouldReadSMBLostLevels(ShouldReadROMHeaderTestCase, TestCase):
    """Check the Super Mario Bros Lost Levels ROM."""

    rom_name = 'super-mario-bros-lost-levels.nes'
    prg_rom_size = 32
    chr_rom_size = 8
    prg_ram_size = 8
    mapper = 0
    is_ignore_mirroring = False
    has_trainer = False
    has_battery_backed_ram = False
    is_vertical_mirroring = True
    has_play_choice_10 = False
    has_vs_unisystem = False
    is_pal = False
    trainer_rom_start = 16
    trainer_rom_stop = 16
    prg_rom_start = 16
    prg_rom_stop = 16 + 32 * 2**10
    chr_rom_start = 16 + 32 * 2**10
    chr_rom_stop = (16 + 32 * 2**10) + (8 * 2**10)


class ShouldReadSuperMarioBros2(ShouldReadROMHeaderTestCase, TestCase):
    """Check the Super Mario Bros 2 ROM."""

    rom_name = 'super-mario-bros-2.nes'
    prg_rom_size = 128
    chr_rom_size = 128
    prg_ram_size = 8
    mapper = 4
    is_ignore_mirroring = False
    has_trainer = False
    has_battery_backed_ram = False
    is_vertical_mirroring = False
    has_play_choice_10 = False
    has_vs_unisystem = False
    is_pal = False
    trainer_rom_start = 16
    trainer_rom_stop = 16
    prg_rom_start = 16
    prg_rom_stop = 16 + 128 * 2**10
    chr_rom_start = 16 + 128 * 2**10
    chr_rom_stop = (16 + 128 * 2**10) + (128 * 2**10)


class ShouldReadSuperMarioBros3(ShouldReadROMHeaderTestCase, TestCase):
    """Check the Super Mario Bros 3 ROM."""

    rom_name = 'super-mario-bros-3.nes'
    prg_rom_size = 256
    chr_rom_size = 128
    prg_ram_size = 8
    mapper = 4
    is_ignore_mirroring = False
    has_trainer = False
    has_battery_backed_ram = False
    is_vertical_mirroring = False
    has_play_choice_10 = False
    has_vs_unisystem = False
    is_pal = False
    trainer_rom_start = 16
    trainer_rom_stop = 16
    prg_rom_start = 16
    prg_rom_stop = 16 + 256 * 2**10
    chr_rom_start = 16 + 256 * 2**10
    chr_rom_stop = (16 + 256 * 2**10) + (128 * 2**10)


class ShouldReadExcitebike(ShouldReadROMHeaderTestCase, TestCase):
    """Check the Excitebike ROM."""

    rom_name = 'excitebike.nes'
    prg_rom_size = 16
    chr_rom_size = 8
    prg_ram_size = 8
    mapper = 0
    is_ignore_mirroring = False
    has_trainer = False
    has_battery_backed_ram = False
    is_vertical_mirroring = True
    has_play_choice_10 = False
    has_vs_unisystem = False
    is_pal = False
    trainer_rom_start = 16
    trainer_rom_stop = 16
    prg_rom_start = 16
    prg_rom_stop = 16 + 16 * 2**10
    chr_rom_start = 16 + 16 * 2**10
    chr_rom_stop = (16 + 16 * 2**10) + (8 * 2**10)


class ShouldReadLegendOfZelda(ShouldReadROMHeaderTestCase, TestCase):
    """Check The Legend Of Zelda ROM."""

    rom_name = 'the-legend-of-zelda.nes'
    prg_rom_size = 128
    chr_rom_size = 0
    prg_ram_size = 8
    mapper = 1
    is_ignore_mirroring = False
    has_trainer = False
    has_battery_backed_ram = True
    is_vertical_mirroring = False
    has_play_choice_10 = False
    has_vs_unisystem = False
    is_pal = False
    trainer_rom_start = 16
    trainer_rom_stop = 16
    prg_rom_start = 16
    prg_rom_stop = 16 + 128 * 2**10
    chr_rom_start = 16 + 128 * 2**10
    chr_rom_stop = (16 + 128 * 2**10) + 0
