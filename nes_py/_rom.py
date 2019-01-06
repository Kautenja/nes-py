"""An abstraction of the NES Read-Only Memory (ROM).

Notes:
    - http://wiki.nesdev.com/w/index.php/INES
"""
import os
import numpy as np


class ROM(object):
    """An abstraction of the NES Read-Only Memory (ROM)."""

    # the magic bytes expected at the first four bytes of the header.
    # It spells "NES<END>"
    _MAGIC = np.array([0x4E, 0x45, 0x53, 0x1A])

    def __init__(self, rom_path):
        """
        Initialize a new ROM.

        Args:
            rom_path (str): the path to the ROM file

        Returns:
            None

        """
        # make sure the rom path is a string
        if not isinstance(rom_path, str):
            raise TypeError('rom_path must be of type: str.')
        # make sure the rom path exists
        if not os.path.exists(rom_path):
            msg = 'rom_path points to non-existent file: {}.'.format(rom_path)
            raise ValueError(msg)
        # read the binary data in the .nes ROM file
        self.raw_data = np.fromfile(rom_path, dtype='uint8')
        # ensure the first 4 bytes are 0x4E45531A (NES<EOF>)
        if not np.array_equal(self._magic, self._MAGIC):
            raise ValueError('ROM missing magic number in header.')
        if self._zero_fill != 0:
            raise ValueError("ROM header zero fill bytes are not zero.")

    #
    # MARK: Header
    #

    @property
    def header(self):
        """Return the header of the ROM file as bytes."""
        return self.raw_data[:16]

    @property
    def _magic(self):
        """Return the magic bytes in the first 4 bytes."""
        return self.header[:4]

    @property
    def prg_rom_size(self):
        """Return the size of the PRG ROM in KB."""
        return 16 * self.header[4]

    @property
    def chr_rom_size(self):
        """Return the size of the CHR ROM in KB."""
        return 8 * self.header[5]

    @property
    def flags_6(self):
        """Return the flags at the 6th byte of the header."""
        return '{:08b}'.format(self.header[6])

    @property
    def flags_7(self):
        """Return the flags at the 7th byte of the header."""
        return '{:08b}'.format(self.header[7])

    @property
    def prg_ram_size(self):
        """Return the size of the PRG RAM in KB."""
        size = self.header[8]
        # size becomes 8 when it's zero for compatibility
        if size == 0:
            size = 1

        return 8 * size

    @property
    def flags_9(self):
        """Return the flags at the 9th byte of the header."""
        return '{:08b}'.format(self.header[9])

    @property
    def flags_10(self):
        """
        Return the flags at the 10th byte of the header.

        Notes:
            - these flags are not part of official specification.
            - ignored in this emulator

        """
        return '{:08b}'.format(self.header[10])

    @property
    def _zero_fill(self):
        """Return the zero fill bytes at the end of the header."""
        return self.header[11:].sum()

    #
    # MARK: Header Flags
    #

    @property
    def mapper(self):
        """Return the mapper number this ROM uses."""
        # the high nibble is in flags 7, the low nibble is in flags 6
        return int(self.flags_7[:4] + self.flags_6[:4], 2)

    @property
    def is_ignore_mirroring(self):
        """Return a boolean determining if the ROM ignores mirroring."""
        return bool(int(self.flags_6[4]))

    @property
    def has_trainer(self):
        """Return a boolean determining if the ROM has a trainer block."""
        return bool(int(self.flags_6[5]))

    @property
    def has_battery_backed_ram(self):
        """Return a boolean determining if the ROM has a battery-backed RAM."""
        return bool(int(self.flags_6[6]))

    @property
    def is_vertical_mirroring(self):
        """Return the mirroring mode this ROM uses."""
        return bool(int(self.flags_6[7]))

    @property
    def has_play_choice_10(self):
        """
        Return whether this cartridge uses PlayChoice-10.

        Note:
            - Play-Choice 10 uses different color palettes for a different PPU
            - ignored in this emulator

        """
        return bool(int(self.flags_7[6]))

    @property
    def has_vs_unisystem(self):
        """
        Return whether this cartridge has VS Uni-system.

        Note:
            VS Uni-system is for ROMs that have a coin slot (Arcades).
            - ignored in this emulator

        """
        return bool(int(self.flags_7[7]))

    @property
    def is_pal(self):
        """Return the TV system this ROM supports."""
        return bool(int(self.flags_9[7]))

    #
    # MARK: ROM
    #

    @property
    def trainer_rom_start(self):
        """The inclusive starting index of the trainer ROM."""
        return 16

    @property
    def trainer_rom_stop(self):
        """The exclusive stopping index of the trainer ROM."""
        if self.has_trainer:
            return 16 + 512
        else:
            return 16

    @property
    def trainer_rom(self):
        """Return the trainer ROM of the ROM file."""
        return self.raw_data[self.trainer_rom_start:self.trainer_rom_stop]

    @property
    def prg_rom_start(self):
        """The inclusive starting index of the PRG ROM."""
        return self.trainer_rom_stop

    @property
    def prg_rom_stop(self):
        """The exclusive stopping index of the PRG ROM."""
        return self.prg_rom_start + self.prg_rom_size * 2**10

    @property
    def prg_rom(self):
        """Return the PRG ROM of the ROM file."""
        try:
            return self.raw_data[self.prg_rom_start:self.prg_rom_stop]
        except IndexError:
            raise ValueError('failed to read PRG-ROM on ROM.')

    @property
    def chr_rom_start(self):
        """The inclusive starting index of the CHR ROM."""
        return self.prg_rom_stop

    @property
    def chr_rom_stop(self):
        """The exclusive stopping index of the CHR ROM."""
        return self.chr_rom_start + self.chr_rom_size * 2**10

    @property
    def chr_rom(self):
        """Return the CHR ROM of the ROM file."""
        try:
            return self.raw_data[self.chr_rom_start:self.chr_rom_stop]
        except IndexError:
            raise ValueError('failed to read CHR-ROM on ROM.')


# explicitly define the outward facing API of this module
__all__ = [ROM.__name__]
