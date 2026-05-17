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
    _HEADER_SIZE = 16
    _TRAINER_SIZE = 512
    _PRG_ROM_BANK_SIZE = 16 * 2**10
    _CHR_ROM_BANK_SIZE = 8 * 2**10
    _PRG_RAM_BANK_SIZE = 8 * 2**10
    _NES2_RAM_GRANULARITY = 64

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
        if len(self.raw_data) < self._HEADER_SIZE:
            raise ValueError('ROM header is truncated.')
        # ensure the first 4 bytes are 0x4E45531A (NES<EOF>)
        if not np.array_equal(self._magic, self._MAGIC):
            raise ValueError('ROM missing magic number in header.')
        if not self.is_nes2 and self._zero_fill != 0:
            raise ValueError("ROM header zero fill bytes are not zero.")
        self._validate_payload()

    @staticmethod
    def _decode_nes2_rom_size(low_byte, high_nibble, unit):
        """Decode a NES 2.0 PRG/CHR ROM size in bytes."""
        if high_nibble == 0x0f:
            exponent = low_byte >> 2
            multiplier = (low_byte & 0x03) * 2 + 1
            return (2 ** exponent) * multiplier
        return ((high_nibble << 8) | low_byte) * unit

    @classmethod
    def _decode_nes2_ram_size(cls, shift_count):
        """Decode a NES 2.0 RAM shift count into bytes."""
        if shift_count == 0:
            return 0
        return cls._NES2_RAM_GRANULARITY << shift_count

    def _validate_payload(self):
        """Validate the trainer, PRG ROM, and CHR ROM payload lengths."""
        if len(self.raw_data) < self.trainer_rom_stop:
            raise ValueError('failed to read trainer on ROM.')
        if len(self.raw_data) < self.prg_rom_stop:
            raise ValueError('failed to read PRG-ROM on ROM.')
        if len(self.raw_data) < self.chr_rom_stop:
            raise ValueError('failed to read CHR-ROM on ROM.')

    def _header_byte(self, index):
        """Return a header byte as a Python integer."""
        return int(self.header[index])

    #
    # MARK: Header
    #

    @property
    def header(self):
        """Return the header of the ROM file as bytes."""
        return self.raw_data[:self._HEADER_SIZE]

    @property
    def _magic(self):
        """Return the magic bytes in the first 4 bytes."""
        return self.header[:4]

    @property
    def prg_rom_size(self):
        """Return the size of the PRG ROM in KB."""
        return self.prg_rom_byte_size // 2**10

    @property
    def prg_rom_byte_size(self):
        """Return the size of the PRG ROM in bytes."""
        if self.is_nes2:
            return self._decode_nes2_rom_size(
                self._header_byte(4),
                self._header_byte(9) & 0x0f,
                self._PRG_ROM_BANK_SIZE
            )
        return self._header_byte(4) * self._PRG_ROM_BANK_SIZE

    @property
    def prg_rom_banks(self):
        """Return the number of 16KB PRG ROM banks."""
        return self.prg_rom_byte_size // self._PRG_ROM_BANK_SIZE

    @property
    def chr_rom_size(self):
        """Return the size of the CHR ROM in KB."""
        return self.chr_rom_byte_size // 2**10

    @property
    def chr_rom_byte_size(self):
        """Return the size of the CHR ROM in bytes."""
        if self.is_nes2:
            return self._decode_nes2_rom_size(
                self._header_byte(5),
                (self._header_byte(9) >> 4) & 0x0f,
                self._CHR_ROM_BANK_SIZE
            )
        return self._header_byte(5) * self._CHR_ROM_BANK_SIZE

    @property
    def chr_rom_banks(self):
        """Return the number of 8KB CHR ROM banks."""
        return self.chr_rom_byte_size // self._CHR_ROM_BANK_SIZE

    @property
    def flags_6(self):
        """Return the flags at the 6th byte of the header."""
        return '{:08b}'.format(int(self.header[6]))

    @property
    def flags_7(self):
        """Return the flags at the 7th byte of the header."""
        return '{:08b}'.format(int(self.header[7]))

    @property
    def prg_ram_size(self):
        """Return the size of the PRG RAM in KB."""
        return self.prg_ram_byte_size // 2**10

    @property
    def prg_ram_byte_size(self):
        """Return the size of ordinary PRG RAM in bytes."""
        if self.is_nes2:
            return self._decode_nes2_ram_size(self._header_byte(10) & 0x0f)
        size = self._header_byte(8)
        # size becomes 8KB when it's zero for compatibility
        if size == 0:
            size = 1
        return size * self._PRG_RAM_BANK_SIZE

    @property
    def prg_battery_ram_byte_size(self):
        """Return the size of battery-backed PRG RAM in bytes."""
        if self.is_nes2:
            return self._decode_nes2_ram_size(
                (self._header_byte(10) >> 4) & 0x0f
            )
        if self._has_battery_backed_ram_flag:
            return self.prg_ram_byte_size
        return 0

    @property
    def chr_ram_byte_size(self):
        """Return the size of ordinary CHR RAM in bytes."""
        if self.is_nes2:
            return self._decode_nes2_ram_size(self._header_byte(11) & 0x0f)
        if self.chr_rom_byte_size == 0:
            return self._CHR_ROM_BANK_SIZE
        return 0

    @property
    def chr_battery_ram_byte_size(self):
        """Return the size of battery-backed CHR RAM in bytes."""
        if self.is_nes2:
            return self._decode_nes2_ram_size(
                (self._header_byte(11) >> 4) & 0x0f
            )
        return 0

    @property
    def flags_9(self):
        """Return the flags at the 9th byte of the header."""
        return '{:08b}'.format(int(self.header[9]))

    @property
    def flags_10(self):
        """
        Return the flags at the 10th byte of the header.

        Notes:
            - these flags are not part of official specification.
            - ignored in this emulator

        """
        return '{:08b}'.format(int(self.header[10]))

    @property
    def _zero_fill(self):
        """Return the zero fill bytes at the end of the header."""
        return int(self.header[11:].sum())

    #
    # MARK: Header Flags
    #

    @property
    def mapper(self):
        """Return the mapper number this ROM uses."""
        mapper = ((self._header_byte(6) >> 4) & 0x0f)
        mapper |= self._header_byte(7) & 0xf0
        if self.is_nes2:
            mapper |= (self._header_byte(8) & 0x0f) << 8
        return mapper

    @property
    def submapper(self):
        """Return the NES 2.0 submapper number, or 0 for iNES."""
        if self.is_nes2:
            return (self._header_byte(8) >> 4) & 0x0f
        return 0

    @property
    def is_nes2(self):
        """Return whether this ROM uses the NES 2.0 header variant."""
        return (self._header_byte(7) & 0x0c) == 0x08

    @property
    def is_ignore_mirroring(self):
        """Return a boolean determining if the ROM ignores mirroring."""
        return bool(self._header_byte(6) & 0x08)

    @property
    def has_trainer(self):
        """Return a boolean determining if the ROM has a trainer block."""
        return bool(self._header_byte(6) & 0x04)

    @property
    def has_battery_backed_ram(self):
        """Return a boolean determining if the ROM has a battery-backed RAM."""
        return bool(
            self._has_battery_backed_ram_flag or
            self.prg_battery_ram_byte_size or
            self.chr_battery_ram_byte_size
        )

    @property
    def _has_battery_backed_ram_flag(self):
        """Return whether the iNES battery-backed RAM flag is set."""
        return bool(self._header_byte(6) & 0x02)

    @property
    def is_vertical_mirroring(self):
        """Return the mirroring mode this ROM uses."""
        return self.mirroring == 'vertical'

    @property
    def mirroring(self):
        """Return the header mirroring mode."""
        if self.is_ignore_mirroring:
            return 'four-screen'
        if self._header_byte(6) & 0x01:
            return 'vertical'
        return 'horizontal'

    @property
    def has_play_choice_10(self):
        """
        Return whether this cartridge uses PlayChoice-10.

        Note:
            - Play-Choice 10 uses different color palettes for a different PPU
            - ignored in this emulator

        """
        return bool(self._header_byte(7) & 0x02)

    @property
    def has_vs_unisystem(self):
        """
        Return whether this cartridge has VS Uni-system.

        Note:
            VS Uni-system is for ROMs that have a coin slot (Arcades).
            - ignored in this emulator

        """
        return bool(self._header_byte(7) & 0x01)

    @property
    def is_pal(self):
        """Return the TV system this ROM supports."""
        if self.is_nes2:
            return (self._header_byte(12) & 0x03) == 0x01
        return bool(self._header_byte(9) & 0x01)

    #
    # MARK: ROM
    #

    @property
    def trainer_rom_start(self):
        """The inclusive starting index of the trainer ROM."""
        return self._HEADER_SIZE

    @property
    def trainer_rom_stop(self):
        """The exclusive stopping index of the trainer ROM."""
        if self.has_trainer:
            return self._HEADER_SIZE + self._TRAINER_SIZE
        return self._HEADER_SIZE

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
        return self.prg_rom_start + self.prg_rom_byte_size

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
        return self.chr_rom_start + self.chr_rom_byte_size

    @property
    def chr_rom(self):
        """Return the CHR ROM of the ROM file."""
        try:
            return self.raw_data[self.chr_rom_start:self.chr_rom_stop]
        except IndexError:
            raise ValueError('failed to read CHR-ROM on ROM.')


# explicitly define the outward facing API of this module
__all__ = [ROM.__name__]
