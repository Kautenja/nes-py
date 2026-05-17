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


def ines_header(
    mapper,
    prg_banks,
    chr_banks,
    mirroring='horizontal',
    trainer=False,
    battery=False,
    vs_unisystem=False,
    play_choice_10=False,
    pal=False,
    prg_ram_banks=0,
    nes2=False,
    submapper=0,
    prg_ram_shift=0,
    prg_battery_ram_shift=0,
    chr_ram_shift=0,
    chr_battery_ram_shift=0,
):
    """Return a minimal iNES header for a synthetic ROM."""
    if mirroring not in {'horizontal', 'vertical', 'four-screen'}:
        raise ValueError('unknown mirroring mode: {}'.format(mirroring))

    flags_6 = (mapper & 0x0f) << 4
    if mirroring == 'vertical':
        flags_6 |= 0x01
    elif mirroring == 'four-screen':
        flags_6 |= 0x08
    if battery or prg_battery_ram_shift or chr_battery_ram_shift:
        flags_6 |= 0x02
    if trainer:
        flags_6 |= 0x04

    flags_7 = mapper & 0xf0
    if vs_unisystem:
        flags_7 |= 0x01
    if play_choice_10:
        flags_7 |= 0x02
    if nes2:
        flags_7 |= 0x08

    header_8 = prg_ram_banks
    header_9 = 0x01 if pal else 0x00
    header_10 = 0x00
    header_11 = 0x00
    header_12 = 0x00
    if nes2:
        header_8 = ((submapper & 0x0f) << 4) | ((mapper >> 8) & 0x0f)
        header_9 = (((chr_banks >> 8) & 0x0f) << 4)
        header_9 |= (prg_banks >> 8) & 0x0f
        header_10 = ((prg_battery_ram_shift & 0x0f) << 4)
        header_10 |= prg_ram_shift & 0x0f
        header_11 = ((chr_battery_ram_shift & 0x0f) << 4)
        header_11 |= chr_ram_shift & 0x0f
        header_12 = 0x01 if pal else 0x00

    return bytes([
        0x4e, 0x45, 0x53, 0x1a,
        prg_banks & 0xff,
        chr_banks & 0xff,
        flags_6,
        flags_7,
        header_8,
        header_9,
        header_10,
        header_11,
        header_12,
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
    trainer=False,
    battery=False,
    vs_unisystem=False,
    play_choice_10=False,
    pal=False,
    prg_ram_banks=0,
    nes2=False,
    submapper=0,
    prg_ram_shift=0,
    prg_battery_ram_shift=0,
    chr_ram_shift=0,
    chr_battery_ram_shift=0,
):
    """Write a synthetic iNES ROM and return its filesystem path."""
    if reset_vector is None:
        reset_vector = 0xc000 if mapper in {1, 2} and prg_banks > 1 else 0x8000

    data = bytearray(ines_header(
        mapper,
        prg_banks,
        chr_banks,
        mirroring,
        trainer=trainer,
        battery=battery,
        vs_unisystem=vs_unisystem,
        play_choice_10=play_choice_10,
        pal=pal,
        prg_ram_banks=prg_ram_banks,
        nes2=nes2,
        submapper=submapper,
        prg_ram_shift=prg_ram_shift,
        prg_battery_ram_shift=prg_battery_ram_shift,
        chr_ram_shift=chr_ram_shift,
        chr_battery_ram_shift=chr_battery_ram_shift,
    ))
    if trainer:
        data.extend(bytes([0xa5] * 512))
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
