# distutils: language = c++
# cython: language_level=3
"""Cython bindings for the native NES emulator core."""

import os
import sys

import numpy as np
cimport numpy as cnp
from libc.stddef cimport size_t
from libc.stdint cimport uint8_t, uint16_t, uint32_t
from libc.stdlib cimport free, malloc
from libcpp cimport bool as cpp_bool
from libcpp.string cimport string


cnp.import_array()


cdef enum:
    SCREEN_WIDTH_C = 256
    SCREEN_HEIGHT_C = 240
    RAM_SIZE_C = 0x800
    RAM_READ_ENCODING_BYTE_CODE = 0
    RAM_READ_ENCODING_LITTLE_CODE = 1
    RAM_READ_ENCODING_BIG_CODE = 2
    RAM_READ_ENCODING_BCD_CODE = 3
    RAM_READ_ENCODING_DIGITS_CODE = 4

SCREEN_WIDTH = SCREEN_WIDTH_C
SCREEN_HEIGHT = SCREEN_HEIGHT_C
RAM_SIZE = RAM_SIZE_C
RAM_READ_ENCODING_BYTE = RAM_READ_ENCODING_BYTE_CODE
RAM_READ_ENCODING_LITTLE = RAM_READ_ENCODING_LITTLE_CODE
RAM_READ_ENCODING_BIG = RAM_READ_ENCODING_BIG_CODE
RAM_READ_ENCODING_BCD = RAM_READ_ENCODING_BCD_CODE
RAM_READ_ENCODING_DIGITS = RAM_READ_ENCODING_DIGITS_CODE


cdef extern from "nes_emu/common.hpp" namespace "NES":
    ctypedef uint8_t NES_Byte
    ctypedef uint16_t NES_Address
    ctypedef uint32_t NES_Pixel


cdef extern from "nes_emu/cartridge.hpp" namespace "NES":
    cdef cppclass CartridgeMetadata:
        uint16_t mapper_number
        NES_Byte submapper
        size_t prg_rom_size
        size_t prg_rom_banks
        size_t chr_rom_size
        size_t chr_rom_banks
        size_t prg_ram_size
        size_t prg_battery_ram_size
        size_t chr_ram_size
        size_t chr_battery_ram_size
        cpp_bool has_trainer
        cpp_bool has_battery
        cpp_bool has_vs_unisystem
        cpp_bool has_play_choice_10
        cpp_bool is_pal
        cpp_bool is_nes2
        size_t trainer_start
        size_t trainer_stop
        NES_Byte name_table_mirroring

    cdef cppclass Cartridge:
        Cartridge() except +
        void loadFromFile(string path, cpp_bool allow_unsupported_features) except +
        const CartridgeMetadata& getMetadata() const


cdef extern from "nes_emu/emulator.hpp" namespace "NES":
    cdef cppclass EmulatorSnapshot "NES::Emulator::Snapshot":
        EmulatorSnapshot() except +
        EmulatorSnapshot(const EmulatorSnapshot&) except +

    cdef cppclass Emulator:
        Emulator(string rom_path) except +
        NES_Pixel* get_screen_buffer() nogil
        NES_Byte* get_memory_buffer() nogil
        NES_Byte* get_controller(int port) nogil
        void reset() except + nogil
        void step() except + nogil
        void backup() except +
        void restore() except +
        EmulatorSnapshot save_state() except +
        EmulatorSnapshot* create_snapshot() except +
        void load_state(const EmulatorSnapshot& snapshot) except +
        void load_snapshot(const EmulatorSnapshot* snapshot) except +
        int get_mapper_number()
        size_t get_prg_rom_size()
        size_t get_chr_rom_size()
        cpp_bool has_chr_ram()
        int get_name_table_mirroring()


cdef extern from "nes_emu/mapper_factory.hpp" namespace "NES":
    cpp_bool IsMapperSupported(uint16_t mapper_id)


cdef string _rom_path_string(object rom_path) except *:
    """Return an OS-encoded path string for the native C++ API."""
    cdef bytes encoded = os.fsencode(rom_path)
    return string(encoded)


cdef object _uint8_array(
    int ndim,
    cnp.npy_intp* dims,
    cnp.npy_intp* strides,
    void* data,
    object base,
):
    """Create a writable NumPy uint8 view over native emulator memory."""
    cdef cnp.ndarray array = cnp.PyArray_New(
        np.ndarray,
        ndim,
        dims,
        cnp.NPY_UINT8,
        strides,
        data,
        0,
        cnp.NPY_ARRAY_WRITEABLE,
        None,
    )
    cnp.set_array_base(array, base)
    return array


cdef cnp.ndarray _validated_uint8_output(object output, tuple shape):
    """Return a writable C-contiguous uint8 output array for native copies."""
    cdef cnp.ndarray array
    cdef object actual_shape
    if output is None:
        return np.empty(shape, dtype=np.uint8)
    array = np.asarray(output)
    actual_shape = np.shape(array)
    if actual_shape != shape:
        raise ValueError(
            'output shape must be {}, got {}'.format(shape, actual_shape)
        )
    if array.dtype != np.uint8:
        raise TypeError(
            'output dtype must be uint8, got {}'.format(array.dtype)
        )
    if not array.flags.c_contiguous:
        raise ValueError('output must be C-contiguous')
    if not array.flags.writeable:
        raise ValueError('output must be writable')
    return array


cdef cnp.ndarray _validated_uint32_output(object output, tuple shape):
    """Return a writable C-contiguous uint32 output array."""
    cdef cnp.ndarray array
    cdef object actual_shape
    if output is None:
        return np.empty(shape, dtype=np.uint32)
    array = np.asarray(output)
    actual_shape = np.shape(array)
    if actual_shape != shape:
        raise ValueError(
            'output shape must be {}, got {}'.format(shape, actual_shape)
        )
    if array.dtype != np.uint32:
        raise TypeError(
            'output dtype must be uint32, got {}'.format(array.dtype)
        )
    if not array.flags.c_contiguous:
        raise ValueError('output must be C-contiguous')
    if not array.flags.writeable:
        raise ValueError('output must be writable')
    return array


cdef void _copy_screen_rgb_pixels(
    NES_Pixel* source,
    uint8_t* out,
) noexcept nogil:
    """Copy a native 32-bit screen buffer into contiguous RGB bytes."""
    cdef size_t index
    cdef size_t out_index
    cdef size_t pixel_count = SCREEN_WIDTH_C * SCREEN_HEIGHT_C
    cdef NES_Pixel pixel
    for index in range(pixel_count):
        pixel = source[index]
        out_index = index * 3
        out[out_index] = <uint8_t>((pixel >> 16) & 0xff)
        out[out_index + 1] = <uint8_t>((pixel >> 8) & 0xff)
        out[out_index + 2] = <uint8_t>(pixel & 0xff)


cdef void _copy_screen_grayscale_pixels(
    NES_Pixel* source,
    uint8_t* out,
) noexcept nogil:
    """Copy a native 32-bit screen buffer into contiguous luma bytes."""
    cdef size_t index
    cdef size_t pixel_count = SCREEN_WIDTH_C * SCREEN_HEIGHT_C
    cdef NES_Pixel pixel
    cdef uint32_t r
    cdef uint32_t g
    cdef uint32_t b
    for index in range(pixel_count):
        pixel = source[index]
        r = (pixel >> 16) & 0xff
        g = (pixel >> 8) & 0xff
        b = pixel & 0xff
        out[index] = <uint8_t>((77 * r + 150 * g + 29 * b) >> 8)


cdef void _read_ram_values_into(
    NES_Byte* ram,
    uint16_t* addresses,
    uint8_t* sizes,
    uint8_t* encodings,
    size_t spec_count,
    uint32_t* out,
) noexcept nogil:
    """Read configured RAM values into a uint32 output row."""
    cdef size_t spec_index
    cdef size_t byte_index
    cdef uint16_t address
    cdef uint8_t size
    cdef uint8_t encoding
    cdef NES_Byte value
    cdef uint32_t decoded
    for spec_index in range(spec_count):
        address = addresses[spec_index]
        size = sizes[spec_index]
        encoding = encodings[spec_index]
        if encoding == RAM_READ_ENCODING_BYTE_CODE:
            out[spec_index] = <uint32_t> ram[address]
        elif encoding == RAM_READ_ENCODING_LITTLE_CODE:
            decoded = 0
            for byte_index in range(size):
                decoded |= (
                    <uint32_t> ram[address + byte_index]
                ) << (8 * byte_index)
            out[spec_index] = decoded
        elif encoding == RAM_READ_ENCODING_BIG_CODE:
            decoded = 0
            for byte_index in range(size):
                decoded = (decoded << 8) | ram[address + byte_index]
            out[spec_index] = decoded
        elif encoding == RAM_READ_ENCODING_BCD_CODE:
            decoded = 0
            for byte_index in range(size):
                value = ram[address + byte_index]
                decoded *= 100
                decoded += ((value >> 4) & 0x0f) * 10 + (value & 0x0f)
            out[spec_index] = decoded
        else:
            decoded = 0
            for byte_index in range(size):
                decoded *= 10
                decoded += ram[address + byte_index]
            out[spec_index] = decoded


cdef cnp.ndarray _validated_ram_addresses(object addresses):
    """Return a C-contiguous uint16 address array."""
    cdef cnp.ndarray array = np.asarray(addresses)
    if array.ndim != 1:
        raise ValueError('addresses must be a one-dimensional array')
    if array.dtype != np.uint16:
        raise TypeError('addresses dtype must be uint16')
    if not array.flags.c_contiguous:
        raise ValueError('addresses must be C-contiguous')
    return array


cdef cnp.ndarray _validated_ram_metadata(object values, str name):
    """Return a C-contiguous uint8 RAM metadata array."""
    cdef cnp.ndarray array = np.asarray(values)
    if array.ndim != 1:
        raise ValueError('{} must be a one-dimensional array'.format(name))
    if array.dtype != np.uint8:
        raise TypeError('{} dtype must be uint8'.format(name))
    if not array.flags.c_contiguous:
        raise ValueError('{} must be C-contiguous'.format(name))
    return array


cdef void _validate_ram_spec_arrays(
    cnp.ndarray addresses,
    cnp.ndarray sizes,
    cnp.ndarray encodings,
) except *:
    """Validate parsed RAM-read spec arrays agree on length."""
    if sizes.shape[0] != addresses.shape[0]:
        raise ValueError('sizes length must match addresses length')
    if encodings.shape[0] != addresses.shape[0]:
        raise ValueError('encodings length must match addresses length')


cdef class NativeStateSnapshot:
    """Opaque Python owner for ``NES::Emulator::Snapshot``."""

    cdef EmulatorSnapshot* _snapshot

    def __cinit__(self):
        self._snapshot = NULL

    def __dealloc__(self):
        if self._snapshot != NULL:
            del self._snapshot
            self._snapshot = NULL

    cdef void _set(self, const EmulatorSnapshot& snapshot) except *:
        if self._snapshot != NULL:
            del self._snapshot
        self._snapshot = new EmulatorSnapshot(snapshot)

    cdef void _set_ptr(self, EmulatorSnapshot* snapshot) except *:
        if snapshot == NULL:
            raise MemoryError()
        if self._snapshot != NULL:
            del self._snapshot
        self._snapshot = snapshot

    cdef const EmulatorSnapshot& _get(self) except *:
        if self._snapshot == NULL:
            raise ValueError('snapshot has not been initialized.')
        return self._snapshot[0]


cdef class NativeEmulator:
    """Python-owned wrapper around ``NES::Emulator``."""

    cdef Emulator* _emu
    cdef bint _closed

    def __cinit__(self, object rom_path):
        cdef string path
        self._emu = NULL
        self._closed = True
        path = _rom_path_string(rom_path)
        self._emu = new Emulator(path)
        self._closed = False

    def __dealloc__(self):
        if self._emu != NULL:
            del self._emu
            self._emu = NULL

    cdef void _require_open(self) except *:
        if self._closed or self._emu == NULL:
            raise ValueError('env has already been closed.')

    def close(self):
        """Close native operations while outstanding buffer views stay valid."""
        self._require_open()
        self._closed = True

    def screen_buffer(self):
        """Return a no-copy RGB view over the native 32-bit screen buffer."""
        self._require_open()
        cdef cnp.npy_intp dims[3]
        cdef cnp.npy_intp strides[3]
        cdef uint8_t* data = <uint8_t*> self._emu.get_screen_buffer()
        dims[0] = SCREEN_HEIGHT
        dims[1] = SCREEN_WIDTH
        dims[2] = 3
        strides[0] = SCREEN_WIDTH * 4
        strides[1] = 4
        if sys.byteorder == 'little':
            data += 2
            strides[2] = -1
        else:
            data += 1
            strides[2] = 1
        return _uint8_array(3, dims, strides, <void*> data, self)

    def copy_screen_rgb(self, object output=None):
        """Copy the current screen to a C-contiguous 24-bit RGB array."""
        self._require_open()
        cdef cnp.ndarray array = _validated_uint8_output(
            output,
            (SCREEN_HEIGHT, SCREEN_WIDTH, 3),
        )
        cdef uint8_t* out = <uint8_t*> cnp.PyArray_DATA(array)
        cdef NES_Pixel* source = self._emu.get_screen_buffer()
        with nogil:
            _copy_screen_rgb_pixels(source, out)
        return array

    def copy_screen_grayscale(self, object output=None):
        """Copy the current screen to a C-contiguous 8-bit luma array."""
        self._require_open()
        cdef cnp.ndarray array = _validated_uint8_output(
            output,
            (SCREEN_HEIGHT, SCREEN_WIDTH),
        )
        cdef uint8_t* out = <uint8_t*> cnp.PyArray_DATA(array)
        cdef NES_Pixel* source = self._emu.get_screen_buffer()
        with nogil:
            _copy_screen_grayscale_pixels(source, out)
        return array

    def ram_buffer(self):
        """Return a no-copy view over the native internal RAM buffer."""
        self._require_open()
        cdef cnp.npy_intp dims[1]
        cdef cnp.npy_intp strides[1]
        dims[0] = RAM_SIZE
        strides[0] = 1
        return _uint8_array(
            1,
            dims,
            strides,
            <void*> self._emu.get_memory_buffer(),
            self,
        )

    def controller_buffer(self, int port):
        """Return a no-copy view over one controller byte."""
        self._require_open()
        if port < 0 or port > 1:
            raise ValueError('controller port must be 0 or 1')
        cdef cnp.npy_intp dims[1]
        cdef cnp.npy_intp strides[1]
        dims[0] = 1
        strides[0] = 1
        return _uint8_array(
            1,
            dims,
            strides,
            <void*> self._emu.get_controller(port),
            self,
        )

    def reset(self):
        """Reset the emulator."""
        self._require_open()
        self._emu.reset()

    def frame_advance(self, object action):
        """Write a controller action and advance one frame."""
        self._require_open()
        self._emu.get_controller(0)[0] = <NES_Byte> int(action)
        with nogil:
            self._emu.step()

    def backup(self):
        """Create a native emulator backup state."""
        self._require_open()
        self._emu.backup()

    def restore(self):
        """Restore the native emulator backup state."""
        self._require_open()
        self._emu.restore()

    def dump_state(self):
        """Return an opaque owned snapshot of the emulator state."""
        self._require_open()
        cdef NativeStateSnapshot snapshot = NativeStateSnapshot()
        snapshot._set_ptr(self._emu.create_snapshot())
        return snapshot

    def load_state(self, object snapshot):
        """Restore the emulator from an opaque state snapshot."""
        self._require_open()
        if not isinstance(snapshot, NativeStateSnapshot):
            raise TypeError('snapshot must be a NativeStateSnapshot')
        (<NativeStateSnapshot> snapshot)._get()
        self._emu.load_snapshot((<NativeStateSnapshot> snapshot)._snapshot)

    def read_ram_values(
        self,
        object addresses,
        object sizes,
        object encodings,
        object output=None,
    ):
        """Read configured RAM values into a reusable uint32 array."""
        self._require_open()
        cdef cnp.ndarray address_array = _validated_ram_addresses(addresses)
        cdef cnp.ndarray size_array = _validated_ram_metadata(sizes, 'sizes')
        cdef cnp.ndarray encoding_array = _validated_ram_metadata(
            encodings,
            'encodings',
        )
        _validate_ram_spec_arrays(address_array, size_array, encoding_array)
        cdef cnp.ndarray out_array = _validated_uint32_output(
            output,
            (address_array.shape[0],),
        )
        cdef uint16_t* address_data = <uint16_t*> cnp.PyArray_DATA(
            address_array
        )
        cdef uint8_t* size_data = <uint8_t*> cnp.PyArray_DATA(size_array)
        cdef uint8_t* encoding_data = <uint8_t*> cnp.PyArray_DATA(
            encoding_array
        )
        cdef uint32_t* out = <uint32_t*> cnp.PyArray_DATA(out_array)
        cdef NES_Byte* ram = self._emu.get_memory_buffer()
        cdef size_t spec_count = <size_t> address_array.shape[0]
        with nogil:
            _read_ram_values_into(
                ram,
                address_data,
                size_data,
                encoding_data,
                spec_count,
                out,
            )
        return out_array

    def mapper_number(self):
        """Return the active mapper number."""
        self._require_open()
        return int(self._emu.get_mapper_number())

    def prg_rom_size(self):
        """Return the loaded PRG ROM size in bytes."""
        self._require_open()
        return int(self._emu.get_prg_rom_size())

    def chr_rom_size(self):
        """Return the loaded CHR ROM size in bytes."""
        self._require_open()
        return int(self._emu.get_chr_rom_size())

    def has_chr_ram(self):
        """Return whether the active mapper uses CHR RAM."""
        self._require_open()
        return bool(self._emu.has_chr_ram())

    def name_table_mirroring(self):
        """Return the active mapper's name table mirroring mode."""
        self._require_open()
        return int(self._emu.get_name_table_mirroring())


cdef class NativeVectorEmulator:
    """Python-owned wrapper around a same-ROM group of native emulators."""

    cdef Emulator** _emus
    cdef size_t _env_count
    cdef bint _closed

    def __cinit__(self, object rom_path, object env_count):
        cdef object requested_count = int(env_count)
        cdef size_t index
        cdef string path
        self._emus = NULL
        self._env_count = 0
        self._closed = True
        if requested_count <= 0:
            raise ValueError('env_count must be positive')
        self._env_count = <size_t> requested_count
        self._emus = <Emulator**> malloc(self._env_count * sizeof(Emulator*))
        if self._emus == NULL:
            self._env_count = 0
            raise MemoryError()
        for index in range(self._env_count):
            self._emus[index] = NULL
        path = _rom_path_string(rom_path)
        try:
            for index in range(self._env_count):
                self._emus[index] = new Emulator(path)
        except Exception:
            self._free_emulators()
            raise
        self._closed = False

    def __dealloc__(self):
        self._free_emulators()

    cdef void _free_emulators(self):
        cdef size_t index
        if self._emus != NULL:
            for index in range(self._env_count):
                if self._emus[index] != NULL:
                    del self._emus[index]
                    self._emus[index] = NULL
            free(self._emus)
            self._emus = NULL
        self._env_count = 0
        self._closed = True

    cdef void _require_open(self) except *:
        if self._closed or self._emus == NULL:
            raise ValueError('vector emulator has already been closed.')

    cdef Py_ssize_t _require_index(self, object index) except -1:
        cdef Py_ssize_t value = int(index)
        if value < 0 or value >= <Py_ssize_t> self._env_count:
            raise IndexError('env index out of range')
        return value

    cdef cnp.ndarray _validated_actions(self, object actions):
        cdef cnp.ndarray array = np.asarray(actions)
        cdef object actual_shape
        if array.ndim != 1:
            raise ValueError('actions must be a one-dimensional uint8 array')
        actual_shape = np.shape(array)
        if array.shape[0] != <Py_ssize_t> self._env_count:
            raise ValueError(
                'actions shape must be ({},), got {}'.format(
                    int(self._env_count),
                    actual_shape,
                )
            )
        if array.dtype != np.uint8:
            raise TypeError('actions dtype must be uint8')
        if not array.flags.c_contiguous:
            raise ValueError('actions must be C-contiguous')
        return array

    def close(self):
        """Close native operations while outstanding views stay valid."""
        self._require_open()
        self._closed = True

    @property
    def env_count(self):
        """Return the number of native emulator instances."""
        return int(self._env_count)

    def screen_buffer(self, object index):
        """Return a no-copy RGB view for one vector slot."""
        self._require_open()
        cdef Py_ssize_t env_index = self._require_index(index)
        cdef cnp.npy_intp dims[3]
        cdef cnp.npy_intp strides[3]
        cdef uint8_t* data = <uint8_t*> (
            self._emus[env_index].get_screen_buffer()
        )
        dims[0] = SCREEN_HEIGHT
        dims[1] = SCREEN_WIDTH
        dims[2] = 3
        strides[0] = SCREEN_WIDTH * 4
        strides[1] = 4
        if sys.byteorder == 'little':
            data += 2
            strides[2] = -1
        else:
            data += 1
            strides[2] = 1
        return _uint8_array(3, dims, strides, <void*> data, self)

    def ram_buffer(self, object index):
        """Return a no-copy RAM view for one vector slot."""
        self._require_open()
        cdef Py_ssize_t env_index = self._require_index(index)
        cdef cnp.npy_intp dims[1]
        cdef cnp.npy_intp strides[1]
        dims[0] = RAM_SIZE
        strides[0] = 1
        return _uint8_array(
            1,
            dims,
            strides,
            <void*> self._emus[env_index].get_memory_buffer(),
            self,
        )

    def reset(self):
        """Reset all emulator instances."""
        self._require_open()
        cdef size_t index
        with nogil:
            for index in range(self._env_count):
                self._emus[index].reset()

    def reset_one(self, object index):
        """Reset one emulator instance."""
        self._require_open()
        cdef Py_ssize_t env_index = self._require_index(index)
        with nogil:
            self._emus[env_index].reset()

    def step(self, object actions):
        """Apply per-env actions and advance each emulator by one frame."""
        self._require_open()
        cdef cnp.ndarray action_array = self._validated_actions(actions)
        cdef uint8_t* action_data = <uint8_t*> cnp.PyArray_DATA(action_array)
        cdef size_t index
        for index in range(self._env_count):
            self._emus[index].get_controller(0)[0] = <NES_Byte> action_data[index]
        with nogil:
            for index in range(self._env_count):
                self._emus[index].step()

    def step_one(self, object index, object action):
        """Apply one action and advance one emulator by one frame."""
        self._require_open()
        cdef Py_ssize_t env_index = self._require_index(index)
        self._emus[env_index].get_controller(0)[0] = <NES_Byte> int(action)
        with nogil:
            self._emus[env_index].step()

    def copy_screen_rgb(self, object index, object output=None):
        """Copy one vector slot screen to a contiguous RGB array."""
        self._require_open()
        cdef Py_ssize_t env_index = self._require_index(index)
        cdef cnp.ndarray array = _validated_uint8_output(
            output,
            (SCREEN_HEIGHT, SCREEN_WIDTH, 3),
        )
        cdef uint8_t* out = <uint8_t*> cnp.PyArray_DATA(array)
        cdef NES_Pixel* source = self._emus[env_index].get_screen_buffer()
        with nogil:
            _copy_screen_rgb_pixels(source, out)
        return array

    def copy_screen_grayscale(self, object index, object output=None):
        """Copy one vector slot screen to a contiguous grayscale array."""
        self._require_open()
        cdef Py_ssize_t env_index = self._require_index(index)
        cdef cnp.ndarray array = _validated_uint8_output(
            output,
            (SCREEN_HEIGHT, SCREEN_WIDTH),
        )
        cdef uint8_t* out = <uint8_t*> cnp.PyArray_DATA(array)
        cdef NES_Pixel* source = self._emus[env_index].get_screen_buffer()
        with nogil:
            _copy_screen_grayscale_pixels(source, out)
        return array

    def copy_screen_rgb_batch(self, object output=None):
        """Copy all screens to a contiguous batch RGB array."""
        self._require_open()
        cdef cnp.ndarray array = _validated_uint8_output(
            output,
            (int(self._env_count), SCREEN_HEIGHT, SCREEN_WIDTH, 3),
        )
        cdef uint8_t* out = <uint8_t*> cnp.PyArray_DATA(array)
        cdef size_t index
        cdef size_t stride = SCREEN_HEIGHT_C * SCREEN_WIDTH_C * 3
        with nogil:
            for index in range(self._env_count):
                _copy_screen_rgb_pixels(
                    self._emus[index].get_screen_buffer(),
                    out + index * stride,
                )
        return array

    def copy_screen_grayscale_batch(self, object output=None):
        """Copy all screens to a contiguous batch grayscale array."""
        self._require_open()
        cdef cnp.ndarray array = _validated_uint8_output(
            output,
            (int(self._env_count), SCREEN_HEIGHT, SCREEN_WIDTH),
        )
        cdef uint8_t* out = <uint8_t*> cnp.PyArray_DATA(array)
        cdef size_t index
        cdef size_t stride = SCREEN_HEIGHT_C * SCREEN_WIDTH_C
        with nogil:
            for index in range(self._env_count):
                _copy_screen_grayscale_pixels(
                    self._emus[index].get_screen_buffer(),
                    out + index * stride,
                )
        return array

    def dump_state(self, object index):
        """Return an opaque snapshot for one vector slot."""
        self._require_open()
        cdef Py_ssize_t env_index = self._require_index(index)
        cdef NativeStateSnapshot snapshot = NativeStateSnapshot()
        snapshot._set_ptr(self._emus[env_index].create_snapshot())
        return snapshot

    def load_state(self, object index, object snapshot):
        """Restore one vector slot from an opaque snapshot."""
        self._require_open()
        cdef Py_ssize_t env_index = self._require_index(index)
        if not isinstance(snapshot, NativeStateSnapshot):
            raise TypeError('snapshot must be a NativeStateSnapshot')
        (<NativeStateSnapshot> snapshot)._get()
        self._emus[env_index].load_snapshot(
            (<NativeStateSnapshot> snapshot)._snapshot
        )

    def read_ram_values(
        self,
        object addresses,
        object sizes,
        object encodings,
        object output=None,
    ):
        """Read configured RAM values for every vector slot."""
        self._require_open()
        cdef cnp.ndarray address_array = _validated_ram_addresses(addresses)
        cdef cnp.ndarray size_array = _validated_ram_metadata(sizes, 'sizes')
        cdef cnp.ndarray encoding_array = _validated_ram_metadata(
            encodings,
            'encodings',
        )
        _validate_ram_spec_arrays(address_array, size_array, encoding_array)
        cdef cnp.ndarray out_array = _validated_uint32_output(
            output,
            (int(self._env_count), address_array.shape[0]),
        )
        cdef uint16_t* address_data = <uint16_t*> cnp.PyArray_DATA(
            address_array
        )
        cdef uint8_t* size_data = <uint8_t*> cnp.PyArray_DATA(size_array)
        cdef uint8_t* encoding_data = <uint8_t*> cnp.PyArray_DATA(
            encoding_array
        )
        cdef uint32_t* out = <uint32_t*> cnp.PyArray_DATA(out_array)
        cdef size_t spec_count = <size_t> address_array.shape[0]
        cdef size_t index
        with nogil:
            for index in range(self._env_count):
                _read_ram_values_into(
                    self._emus[index].get_memory_buffer(),
                    address_data,
                    size_data,
                    encoding_data,
                    spec_count,
                    out + index * spec_count,
                )
        return out_array


def cartridge_error(object rom_path):
    """Return the native cartridge validation error for a ROM path, if any."""
    cdef Cartridge cartridge
    try:
        cartridge.loadFromFile(_rom_path_string(rom_path), False)
    except Exception as error:
        return str(error)
    return None


def cartridge_metadata(object rom_path):
    """Return parsed native cartridge metadata for a ROM path."""
    cdef Cartridge cartridge
    cdef const CartridgeMetadata* metadata
    cartridge.loadFromFile(_rom_path_string(rom_path), True)
    metadata = &cartridge.getMetadata()
    return {
        'mapper': int(metadata.mapper_number),
        'submapper': int(metadata.submapper),
        'prg_rom_byte_size': int(metadata.prg_rom_size),
        'prg_rom_banks': int(metadata.prg_rom_banks),
        'chr_rom_byte_size': int(metadata.chr_rom_size),
        'chr_rom_banks': int(metadata.chr_rom_banks),
        'prg_ram_byte_size': int(metadata.prg_ram_size),
        'prg_battery_ram_byte_size': int(metadata.prg_battery_ram_size),
        'chr_ram_byte_size': int(metadata.chr_ram_size),
        'chr_battery_ram_byte_size': int(metadata.chr_battery_ram_size),
        'has_trainer': bool(metadata.has_trainer),
        'trainer_rom_start': int(metadata.trainer_start),
        'trainer_rom_stop': int(metadata.trainer_stop),
        'has_battery_backed_ram': bool(metadata.has_battery),
        'name_table_mirroring': int(metadata.name_table_mirroring),
        'has_vs_unisystem': bool(metadata.has_vs_unisystem),
        'has_play_choice_10': bool(metadata.has_play_choice_10),
        'is_pal': bool(metadata.is_pal),
        'is_nes2': bool(metadata.is_nes2),
    }


def is_mapper_supported(object mapper):
    """Return whether a mapper ID has a native implementation."""
    return bool(IsMapperSupported(<uint16_t> int(mapper)))
