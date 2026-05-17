# distutils: language = c++
# cython: language_level=3
"""Cython bindings for the native NES emulator core."""

import os
import sys

import numpy as np
cimport numpy as cnp
from libc.stddef cimport size_t
from libc.stdint cimport uint8_t, uint16_t, uint32_t
from libcpp cimport bool as cpp_bool
from libcpp.string cimport string


cnp.import_array()


SCREEN_WIDTH = 256
SCREEN_HEIGHT = 240
RAM_SIZE = 0x800


cdef extern from "common.hpp" namespace "NES":
    ctypedef uint8_t NES_Byte
    ctypedef uint16_t NES_Address
    ctypedef uint32_t NES_Pixel


cdef extern from "cartridge.hpp" namespace "NES":
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


cdef extern from "emulator.hpp" namespace "NES":
    cdef cppclass Emulator:
        Emulator(string rom_path) except +
        NES_Pixel* get_screen_buffer()
        NES_Byte* get_memory_buffer()
        NES_Byte* get_controller(int port)
        void reset() except +
        void step() except + nogil
        void backup() except +
        void restore() except +
        int get_mapper_number()
        size_t get_prg_rom_size()
        size_t get_chr_rom_size()
        cpp_bool has_chr_ram()
        int get_name_table_mirroring()
        NES_Byte read_prg(NES_Address address)
        void write_prg(NES_Address address, NES_Byte value)
        NES_Byte read_chr(NES_Address address)
        void write_chr(NES_Address address, NES_Byte value)


cdef extern from "mapper_factory.hpp" namespace "NES":
    cpp_bool IsMapperSupported(uint16_t mapper_id)


cdef extern from *:
    """
    extern "C" int MapperIRQSmokeTest();
    extern "C" int MapperCPUCycleHookSmokeTest();
    extern "C" int MapperPPUHookSmokeTest();
    extern "C" int MapperExpansionSmokeTest();
    extern "C" int MapperPRGRAMSmokeTest();
    extern "C" int MapperNameTableSmokeTest();
    extern "C" unsigned int MapperBankHelperSmokeResults();
    """
    int MapperIRQSmokeTest()
    int MapperCPUCycleHookSmokeTest()
    int MapperPPUHookSmokeTest()
    int MapperExpansionSmokeTest()
    int MapperPRGRAMSmokeTest()
    int MapperNameTableSmokeTest()
    unsigned int MapperBankHelperSmokeResults()


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

    def read_prg(self, object address):
        """Read one byte through the active mapper PRG path."""
        self._require_open()
        return int(self._emu.read_prg(<NES_Address> int(address)))

    def write_prg(self, object address, object value):
        """Write one byte through the active mapper PRG path."""
        self._require_open()
        self._emu.write_prg(
            <NES_Address> int(address),
            <NES_Byte> int(value),
        )

    def read_chr(self, object address):
        """Read one byte through the active mapper CHR path."""
        self._require_open()
        return int(self._emu.read_chr(<NES_Address> int(address)))

    def write_chr(self, object address, object value):
        """Write one byte through the active mapper CHR path."""
        self._require_open()
        self._emu.write_chr(
            <NES_Address> int(address),
            <NES_Byte> int(value),
        )


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


def mapper_hook_smoke_results():
    """Run focused native mapper hook smoke checks."""
    return {
        'irq': bool(MapperIRQSmokeTest()),
        'cpu_cycle': bool(MapperCPUCycleHookSmokeTest()),
        'ppu': bool(MapperPPUHookSmokeTest()),
        'expansion': bool(MapperExpansionSmokeTest()),
        'prg_ram': bool(MapperPRGRAMSmokeTest()),
        'nametable': bool(MapperNameTableSmokeTest()),
    }


def mapper_bank_helper_smoke_results():
    """Run focused native mapper bank helper smoke checks."""
    cdef unsigned int results = MapperBankHelperSmokeResults()
    return {
        'prg_8k': bool(results & (1 << 0)),
        'prg_16k': bool(results & (1 << 1)),
        'prg_32k': bool(results & (1 << 2)),
        'chr_1k': bool(results & (1 << 3)),
        'chr_2k': bool(results & (1 << 4)),
        'chr_4k': bool(results & (1 << 5)),
        'chr_8k': bool(results & (1 << 6)),
        'masks_and_bus_conflicts': bool(results & (1 << 7)),
    }
