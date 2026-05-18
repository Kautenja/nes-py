"""Generic RAM read descriptors for NES training-loop helpers."""
import numpy as np

from . import _native


RAM_READ_ENCODINGS = {
    'byte': _native.RAM_READ_ENCODING_BYTE,
    'little': _native.RAM_READ_ENCODING_LITTLE,
    'big': _native.RAM_READ_ENCODING_BIG,
    'bcd': _native.RAM_READ_ENCODING_BCD,
    'digits': _native.RAM_READ_ENCODING_DIGITS,
}


def _coerce_int(name, value):
    """Return ``value`` as an integer or raise a helpful error."""
    try:
        return int(value)
    except (TypeError, ValueError) as error:
        raise TypeError('{} must be an integer'.format(name)) from error


def _normalize_mapping(spec):
    """Return address, size, and encoding from a mapping descriptor."""
    if 'address' not in spec:
        raise ValueError('RAM read mapping specs require an address')
    return (
        spec['address'],
        spec.get('size', 1),
        spec.get('encoding', 'byte'),
    )


def _normalize_sequence(spec):
    """Return address, size, and encoding from a sequence descriptor."""
    if len(spec) == 2:
        return spec[0], spec[1], 'little'
    if len(spec) == 3:
        return spec[0], spec[1], spec[2]
    raise ValueError(
        'RAM read tuple specs must be (address, size) or '
        '(address, size, encoding)'
    )


def _normalize_one(spec):
    """Return one validated RAM read descriptor tuple."""
    if isinstance(spec, dict):
        address, size, encoding = _normalize_mapping(spec)
    elif isinstance(spec, (int, np.integer)):
        address, size, encoding = spec, 1, 'byte'
    else:
        try:
            address, size, encoding = _normalize_sequence(tuple(spec))
        except TypeError as error:
            raise TypeError(
                'RAM read specs must be integers, mappings, or tuples'
            ) from error

    address = _coerce_int('address', address)
    size = _coerce_int('size', size)
    encoding = str(encoding).lower()
    if encoding not in RAM_READ_ENCODINGS:
        raise ValueError('unknown RAM read encoding: {}'.format(encoding))
    if address < 0 or address >= _native.RAM_SIZE:
        raise ValueError('address must be in [0, 0x800)')
    if size <= 0:
        raise ValueError('size must be positive')
    if address + size > _native.RAM_SIZE:
        raise ValueError('RAM read extends past 0x800 bytes')
    if encoding == 'byte' and size != 1:
        raise ValueError('byte RAM reads must use size 1')
    if encoding in {'little', 'big', 'bcd'} and size > 4:
        raise ValueError('{} RAM reads support sizes up to 4'.format(
            encoding
        ))
    if encoding == 'digits' and size > 9:
        raise ValueError('digits RAM reads support sizes up to 9')
    return address, size, RAM_READ_ENCODINGS[encoding]


def normalize_ram_read_specs(specs):
    """
    Normalize public RAM read descriptors for native batch reads.

    Supported descriptors are:

    - ``address``: read one byte.
    - ``(address, size)``: read a little-endian unsigned integer.
    - ``(address, size, encoding)``: read using ``byte``, ``little``, ``big``,
      ``bcd``, or ``digits``.
    - ``{"address": ..., "size": ..., "encoding": ...}``: mapping form of
      the tuple descriptor.
    """
    if specs is None:
        raise TypeError('RAM read specs are required')
    try:
        normalized = [_normalize_one(spec) for spec in specs]
    except TypeError as error:
        raise TypeError('RAM read specs must be iterable') from error

    count = len(normalized)
    addresses = np.empty(count, dtype=np.uint16)
    sizes = np.empty(count, dtype=np.uint8)
    encodings = np.empty(count, dtype=np.uint8)
    for index, (address, size, encoding) in enumerate(normalized):
        addresses[index] = address
        sizes[index] = size
        encodings[index] = encoding
    return addresses, sizes, encodings


__all__ = [
    'RAM_READ_ENCODINGS',
    'normalize_ram_read_specs',
]
