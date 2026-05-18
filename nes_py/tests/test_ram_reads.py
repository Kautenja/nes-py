"""Tests for generic RAM batch read helpers."""
from unittest import TestCase

import numpy as np

from nes_py.nes_env import NESEnv
from nes_py.tests.rom_file_abs_path import rom_file_abs_path
from nes_py.vector_env import VectorNESEmulator


def create_env():
    """Return a reset SMB1 environment for RAM helper tests."""
    env = NESEnv(rom_file_abs_path('super-mario-bros-1.nes'))
    env.reset(seed=5)
    return env


class ShouldReadConfiguredRAMValues(TestCase):
    """Exercise byte, integer, and digit-oriented RAM reads."""

    def test_scalar_reads_values_and_reuses_output(self):
        env = create_env()
        try:
            env.ram[0x10] = 7
            env.ram[0x11] = 0x34
            env.ram[0x12] = 0x12
            env.ram[0x13] = 0x56
            env.ram[0x14] = 0x78
            env.ram[0x15] = 0x12
            env.ram[0x16] = 0x34
            env.ram[0x17] = 1
            env.ram[0x18] = 2
            env.ram[0x19] = 3
            specs = (
                0x10,
                (0x11, 2, 'little'),
                (0x13, 2, 'big'),
                (0x15, 2, 'bcd'),
                {'address': 0x17, 'size': 3, 'encoding': 'digits'},
            )
            output = np.empty((5,), dtype=np.uint32)

            values = env.ram_values(specs, output=output)

            self.assertIs(output, values)
            self.assertEqual(
                [7, 0x1234, 0x5678, 1234, 123],
                values.tolist(),
            )
        finally:
            env.close()

    def test_empty_specs_return_empty_uint32_array(self):
        env = create_env()
        try:
            values = env.ram_values(())
            self.assertEqual((0,), values.shape)
            self.assertEqual(np.uint32, values.dtype)
        finally:
            env.close()

    def test_validates_specs_and_output(self):
        env = create_env()
        try:
            for specs in (
                (-1,),
                (0x800,),
                ((0x7ff, 2),),
                ((0x10, 0),),
                ((0x10, 2, 'byte'),),
                ((0x10, 5, 'little'),),
                ((0x10, 1, 'unknown'),),
            ):
                with self.subTest(specs=specs):
                    with self.assertRaises((TypeError, ValueError)):
                        env.ram_values(specs)
            with self.assertRaises(ValueError):
                env.ram_values((0x10,), output=np.empty((2,), dtype=np.uint32))
            with self.assertRaises(TypeError):
                env.ram_values((0x10,), output=np.empty((1,), dtype=np.uint8))
            non_contiguous = np.empty((4,), dtype=np.uint32)[::2]
            with self.assertRaises(ValueError):
                env.ram_values(
                    (0x10, 0x11),
                    output=non_contiguous,
                )
        finally:
            env.close()

    def test_reset_restore_and_close_behavior(self):
        env = create_env()
        try:
            env.ram[0x20] = 11
            env._backup()
            env.ram[0x20] = 99
            self.assertEqual([99], env.ram_values((0x20,)).tolist())
            env._restore()
            self.assertEqual([11], env.ram_values((0x20,)).tolist())
            env.reset(seed=5)
            self.assertEqual((1,), env.ram_values((0x20,)).shape)
            output = env.ram_values((0x20,))
        finally:
            env.close()

        self.assertEqual((1,), output.shape)
        with self.assertRaises(ValueError):
            env.ram_values((0x20,))

    def test_vector_reads_all_slots(self):
        vector = VectorNESEmulator(
            rom_file_abs_path('super-mario-bros-1.nes'),
            2,
        )
        try:
            vector.reset()
            vector.rams[0][0x30] = 1
            vector.rams[1][0x30] = 2
            output = np.empty((2, 1), dtype=np.uint32)
            values = vector.ram_values((0x30,), output=output)

            self.assertIs(output, values)
            self.assertEqual([[1], [2]], values.tolist())
        finally:
            vector.close()
