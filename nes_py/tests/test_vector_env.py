"""Tests for the opt-in native vector emulator."""
from unittest import TestCase

import numpy as np

from nes_py.nes_env import NESEnv
from nes_py.nes_env import OBSERVATION_MODE_GRAYSCALE
from nes_py.nes_env import OBSERVATION_MODE_RGB_ARRAY_CONTIGUOUS
from nes_py.nes_env import SCREEN_SHAPE_24_BIT
from nes_py.nes_env import SCREEN_SHAPE_GRAYSCALE
from nes_py.tests.rom_file_abs_path import rom_file_abs_path
from nes_py.vector_env import VectorNESEmulator


REPRESENTATIVE_MAPPER_ROMS = (
    'super-mario-bros-1.nes',
    'the-legend-of-zelda.nes',
    'mega-man.nes',
    'adventure-island.nes',
    'super-mario-bros-3.nes',
    'castlevania-iii-draculas-curse.nes',
    'battletoads.nes',
    'mike-tysons-punch-out.nes',
    'batman-return-of-the-joker.nes',
)


class ShouldRunNativeVectorEmulator(TestCase):
    """Exercise vector construction, stepping, buffers, and close semantics."""

    def test_construct_reset_step_reset_one_and_close(self):
        vector = VectorNESEmulator(
            rom_file_abs_path('super-mario-bros-1.nes'),
            3,
        )
        try:
            self.assertEqual(3, vector.num_envs)
            observations = vector.reset()
            self.assertEqual(3, len(observations))
            for screen, ram in zip(vector.screens, vector.rams):
                self.assertEqual(SCREEN_SHAPE_24_BIT, screen.shape)
                self.assertEqual((0x800,), ram.shape)

            vector.step(np.array([0, 1, 2], dtype=np.uint8))
            vector.reset_one(1)
            one = vector.step_one(1, 8)
            self.assertEqual(SCREEN_SHAPE_24_BIT, one.shape)
        finally:
            screen = vector.screens[0]
            ram = vector.rams[0]
            vector.close()

        self.assertEqual(SCREEN_SHAPE_24_BIT, screen.shape)
        self.assertEqual((0x800,), ram.shape)
        with self.assertRaises(ValueError):
            vector.step(np.array([0, 0, 0], dtype=np.uint8))
        self.assertEqual(3, len(vector.observation()))

    def test_copy_observation_modes_can_reuse_output(self):
        vector = VectorNESEmulator(
            rom_file_abs_path('super-mario-bros-1.nes'),
            2,
        )
        try:
            vector.reset()
            vector.step(np.array([0, 8], dtype=np.uint8))
            rgb = np.empty((2,) + SCREEN_SHAPE_24_BIT, dtype=np.uint8)
            gray = np.empty((2,) + SCREEN_SHAPE_GRAYSCALE, dtype=np.uint8)

            rgb_result = vector.observation(
                OBSERVATION_MODE_RGB_ARRAY_CONTIGUOUS,
                output=rgb,
            )
            gray_result = vector.observation(
                OBSERVATION_MODE_GRAYSCALE,
                output=gray,
            )

            self.assertIs(rgb, rgb_result)
            self.assertIs(gray, gray_result)
            self.assertEqual((2,) + SCREEN_SHAPE_24_BIT, rgb.shape)
            self.assertEqual((2,) + SCREEN_SHAPE_GRAYSCALE, gray.shape)
            self.assertTrue(np.array_equal(
                np.ascontiguousarray(vector.screens[0]),
                rgb[0],
            ))
            self.assertEqual(np.uint8, gray.dtype)
        finally:
            vector.close()

    def test_invalid_inputs_raise_clear_errors(self):
        vector = VectorNESEmulator(
            rom_file_abs_path('super-mario-bros-1.nes'),
            2,
        )
        try:
            with self.assertRaises(ValueError):
                vector.step(np.zeros((2, 1), dtype=np.uint8))
            with self.assertRaises(TypeError):
                vector.step(np.array([0, 1], dtype=np.int64))
            with self.assertRaises(ValueError):
                vector.step(np.array([0], dtype=np.uint8))
            with self.assertRaises(IndexError):
                vector.reset_one(2)
            with self.assertRaises(IndexError):
                vector.observation_one(-1)
            with self.assertRaises(NotImplementedError):
                vector.observation('nearest_neighbor')
        finally:
            vector.close()

        with self.assertRaises(ValueError):
            VectorNESEmulator(rom_file_abs_path('missing.nes'), 2)
        with self.assertRaises(ValueError):
            VectorNESEmulator(rom_file_abs_path('super-mario-bros-1.nes'), 0)

    def test_vector_steps_match_scalar_envs_for_representative_mappers(self):
        actions = (
            np.array([0, 1, 2], dtype=np.uint8),
            np.array([4, 8, 16], dtype=np.uint8),
            np.array([32, 64, 128], dtype=np.uint8),
            np.array([255, 0, 8], dtype=np.uint8),
        )
        for name in REPRESENTATIVE_MAPPER_ROMS:
            with self.subTest(name=name):
                rom = rom_file_abs_path(name)
                scalars = [NESEnv(rom) for _ in range(3)]
                vector = VectorNESEmulator(rom, 3)
                try:
                    for scalar in scalars:
                        scalar.reset(seed=31)
                    vector.reset(seed=31)

                    for action_row in actions:
                        vector.step(action_row)
                        for index, scalar in enumerate(scalars):
                            scalar.step(int(action_row[index]))
                            self.assertTrue(np.array_equal(
                                scalar.screen,
                                vector.screens[index],
                            ))
                            self.assertTrue(np.array_equal(
                                scalar.ram,
                                vector.rams[index],
                            ))
                finally:
                    for scalar in scalars:
                        scalar.close()
                    vector.close()
