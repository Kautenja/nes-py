"""Synthetic iNES ROM helpers for native mapper characterization tests."""
from pathlib import Path


PRG_BANK_SIZE = 0x4000
CHR_BANK_SIZE = 0x2000


def prg_bank_marker(bank):
    """Return the stable marker byte used for a PRG bank."""
    return ((bank + 1) * 0x11) & 0xff


def chr_bank_marker(bank):
    """Return the stable marker byte used for a CHR bank."""
    return (0x80 + bank) & 0xff


def ines_header(mapper, prg_banks, chr_banks, mirroring='horizontal'):
    """Return a minimal iNES header for a synthetic ROM."""
    if mirroring not in {'horizontal', 'vertical', 'four-screen'}:
        raise ValueError('unknown mirroring mode: {}'.format(mirroring))

    flags_6 = (mapper & 0x0f) << 4
    if mirroring == 'vertical':
        flags_6 |= 0x01
    elif mirroring == 'four-screen':
        flags_6 |= 0x08

    return bytes([
        0x4e, 0x45, 0x53, 0x1a,
        prg_banks,
        chr_banks,
        flags_6,
        mapper & 0xf0,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
    ])


def synthetic_rom_path(
    directory,
    name,
    mapper,
    prg_banks,
    chr_banks,
    mirroring='horizontal',
    reset_vector=None,
):
    """Write a synthetic iNES ROM and return its filesystem path."""
    if reset_vector is None:
        reset_vector = 0xc000 if mapper in {1, 2} and prg_banks > 1 else 0x8000

    data = bytearray(ines_header(mapper, prg_banks, chr_banks, mirroring))
    prg = bytearray()
    for bank in range(prg_banks):
        bank_data = bytearray([prg_bank_marker(bank)] * PRG_BANK_SIZE)
        bank_data[:4] = bytes([
            0xea,
            0x4c,
            reset_vector & 0xff,
            (reset_vector >> 8) & 0xff,
        ])
        prg.extend(bank_data)

    vector_offset = len(prg) - 4
    prg[vector_offset] = reset_vector & 0xff
    prg[vector_offset + 1] = (reset_vector >> 8) & 0xff
    data.extend(prg)

    for bank in range(chr_banks):
        data.extend(bytes([chr_bank_marker(bank)] * CHR_BANK_SIZE))

    path = Path(directory) / name
    path.write_bytes(data)
    return str(path)
