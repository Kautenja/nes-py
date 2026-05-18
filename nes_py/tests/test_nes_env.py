"""Application-level test cases for the NESEnv class."""
from unittest import mock
from unittest import TestCase

import gymnasium as gym
import numpy as np

from nes_py.tests.rom_file_abs_path import rom_file_abs_path
from nes_py.nes_env import NESEnv
from nes_py.nes_env import OBSERVATION_MODE_GRAYSCALE
from nes_py.nes_env import OBSERVATION_MODE_RGB_ARRAY
from nes_py.nes_env import OBSERVATION_MODE_RGB_ARRAY_CONTIGUOUS
from nes_py.nes_env import SCREEN_SHAPE_24_BIT
from nes_py.nes_env import SCREEN_SHAPE_GRAYSCALE


USABLE_ON_DISK_ROM_NAMES = (
    'super-mario-bros-1.nes',
    'super-mario-bros-lost-levels.nes',
    'the-legend-of-zelda.nes',
    'excitebike.nes',
    'super-mario-bros-2.nes',
    'super-mario-bros-3.nes',
)

UNSUPPORTED_ON_DISK_ROM_NAMES = ()

DETERMINISTIC_ACTIONS = (0, 1, 2, 4, 8, 16, 32, 64)
SNAPSHOT_MAPPER_ROM_NAMES = (
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


def create_smb1_instance():
    """Return a new SMB1 instance."""
    return NESEnv(rom_file_abs_path('super-mario-bros-1.nes'))


class TerminatingNESEnv(NESEnv):
    """NESEnv test subclass that terminates after one step."""

    def _get_terminated(self):
        """Return True after every step."""
        return True


class LegacyDoneNESEnv(NESEnv):
    """NESEnv test subclass that uses the deprecated done hook."""

    def _get_done(self):
        """Return True after every step."""
        return True


class NESEnvApplicationAssertions(TestCase):
    """Shared assertions for public NESEnv workflows."""

    def assert_valid_frame(self, state):
        """Assert an observation has the public NES RGB frame contract."""
        self.assertIsInstance(state, np.ndarray)
        self.assertEqual(SCREEN_SHAPE_24_BIT, state.shape)
        self.assertEqual(np.uint8, state.dtype)

    def advance(self, env, actions):
        """Advance an environment and capture public step outputs."""
        outputs = []
        for action in actions:
            state, reward, terminated, truncated, info = env.step(action)
            outputs.append((
                state.copy(),
                reward,
                terminated,
                truncated,
                info.copy(),
            ))
        return outputs

    def reset_frame(self, env, **kwargs):
        """Reset an environment and assert the Gymnasium reset contract."""
        output = env.reset(**kwargs)
        self.assertIsInstance(output, tuple)
        self.assertEqual(2, len(output))
        state, info = output
        self.assert_valid_frame(state)
        self.assertIsInstance(info, dict)
        return state, info

    def step_frame(self, env, action):
        """Step an environment and assert the Gymnasium step contract."""
        output = env.step(action)
        self.assertIsInstance(output, tuple)
        self.assertEqual(5, len(output))
        state, reward, terminated, truncated, info = output
        self.assert_valid_frame(state)
        self.assertIsInstance(reward, float)
        self.assertIsInstance(terminated, bool)
        self.assertIsInstance(truncated, bool)
        self.assertFalse(truncated)
        self.assertIsInstance(info, dict)
        return state, reward, terminated, truncated, info


class ShouldRaiseTypeErrorOnInvalidROMPathType(TestCase):
    def test(self):
        self.assertRaises(TypeError, NESEnv, 0)


class ShouldRaiseValueErrorOnMissingNonexistentROMFile(TestCase):
    def test(self):
        path = rom_file_abs_path('missing.nes')
        self.assertRaises(ValueError, NESEnv, path)


class ShouldRaiseValueErrorOnNonexistentFile(TestCase):
    def test(self):
        self.assertRaises(ValueError, NESEnv, 'not_a_file.nes')


class ShouldRaiseValueErrorOnNoniNES_ROMPath(TestCase):
    def test(self):
        self.assertRaises(ValueError, NESEnv, rom_file_abs_path('blank'))


class ShouldRaiseValueErrorOnInvalidiNES_ROMPath(TestCase):
    def test(self):
        self.assertRaises(ValueError, NESEnv, rom_file_abs_path('empty.nes'))


class ShouldRaiseErrorOnStepBeforeReset(TestCase):
    def test(self):
        env = create_smb1_instance()
        try:
            self.assertRaises(ValueError, env.step, 0)
        finally:
            env.close()


class ShouldCreateInstanceOfNESEnv(TestCase):
    def test(self):
        env = create_smb1_instance()
        try:
            self.assertIsInstance(env, gym.Env)
        finally:
            env.close()


class ShouldExerciseOnDiskUsableROMs(NESEnvApplicationAssertions):
    """Exercise every on-disk ROM supported by the current native mappers."""

    def test_construct_reset_step_render_and_close(self):
        for name in USABLE_ON_DISK_ROM_NAMES:
            with self.subTest(name=name):
                env = NESEnv(rom_file_abs_path(name), render_mode='rgb_array')
                try:
                    self.assertIsInstance(env, gym.Env)
                    self.assertIn('rgb_array', env.metadata['render_modes'])
                    self.assertIn('human', env.metadata['render_modes'])
                    self.assertNotIn('render.modes', env.metadata)
                    self.assertEqual(256, env.action_space.n)
                    self.assertEqual(SCREEN_SHAPE_24_BIT,
                                     env.observation_space.shape)
                    self.assertEqual(np.uint8, env.observation_space.dtype)

                    self.reset_frame(env, seed=17)
                    for action in DETERMINISTIC_ACTIONS:
                        self.step_frame(env, action)

                    render = env.render()
                    self.assertIs(render, env.screen)
                    self.assert_valid_frame(render)
                finally:
                    env.close()

    def test_unsupported_on_disk_mapper_roms_reject_through_constructor(self):
        for name in UNSUPPORTED_ON_DISK_ROM_NAMES:
            with self.subTest(name=name):
                with self.assertRaises(ValueError) as error:
                    NESEnv(rom_file_abs_path(name))
                self.assertIn('unsupported mapper number 4',
                              str(error.exception))


class ShouldExposeMutableApplicationBuffers(NESEnvApplicationAssertions):
    def assert_native_view(self, array, shape):
        """Assert a public buffer view has the expected NumPy contract."""
        self.assertIsInstance(array, np.ndarray)
        self.assertEqual(shape, array.shape)
        self.assertEqual(np.uint8, array.dtype)
        self.assertTrue(array.flags.writeable)

    def test_screen_ram_and_controller_views_keep_layout_across_operations(self):
        env = create_smb1_instance()
        screen = env.screen
        ram = env.ram
        controller = env.controllers[0]

        self.assert_native_view(screen, SCREEN_SHAPE_24_BIT)
        self.assert_native_view(ram, (0x800,))
        self.assert_native_view(controller, (1,))
        self.assertEqual((SCREEN_SHAPE_24_BIT[1] * 4, 4, -1),
                         screen.strides)

        ram[0x0776] = 0x2a
        controller[0] = 0x81
        self.assertEqual(0x2a, ram[0x0776])
        self.assertEqual(0x81, controller[0])

        self.reset_frame(env)
        self.step_frame(env, 1)

        self.assertIs(screen, env.screen)
        self.assertIs(ram, env.ram)
        self.assertIs(controller, env.controllers[0])
        self.assert_native_view(env.screen, SCREEN_SHAPE_24_BIT)
        self.assert_native_view(env.ram, (0x800,))
        self.assert_native_view(env.controllers[0], (1,))
        env.close()
        self.assert_native_view(screen, SCREEN_SHAPE_24_BIT)
        self.assert_native_view(ram, (0x800,))
        self.assert_native_view(controller, (1,))
        self.assertEqual(1, controller[0])


class ShouldExposeObservationFastPaths(NESEnvApplicationAssertions):
    """Exercise opt-in ML observation copy helpers."""

    def assert_contiguous_uint8(self, array, shape):
        """Assert an observation helper returned a C-contiguous uint8 array."""
        self.assertIsInstance(array, np.ndarray)
        self.assertEqual(shape, array.shape)
        self.assertEqual(np.uint8, array.dtype)
        self.assertTrue(array.flags.c_contiguous)
        self.assertTrue(array.flags.writeable)

    def expected_grayscale(self, frame):
        """Return the integer luma conversion used by the native helper."""
        return ((
            77 * frame[..., 0].astype(np.uint16) +
            150 * frame[..., 1].astype(np.uint16) +
            29 * frame[..., 2].astype(np.uint16)
        ) >> 8).astype(np.uint8)

    def test_default_mode_returns_public_zero_copy_screen(self):
        env = create_smb1_instance()
        try:
            self.reset_frame(env, seed=19)
            self.assertIs(env.screen, env.observation())
            self.assertIs(
                env.screen,
                env.observation(OBSERVATION_MODE_RGB_ARRAY),
            )
        finally:
            env.close()

    def test_copy_modes_match_current_screen_and_can_reuse_output(self):
        env = create_smb1_instance()
        try:
            self.reset_frame(env, seed=19)
            rgb_output = np.empty(SCREEN_SHAPE_24_BIT, dtype=np.uint8)
            gray_output = np.empty(SCREEN_SHAPE_GRAYSCALE, dtype=np.uint8)

            for action in (0, 8, 0):
                env.step(action)
                expected_rgb = np.ascontiguousarray(env.screen)
                expected_gray = self.expected_grayscale(expected_rgb)

                rgb = env.observation(
                    OBSERVATION_MODE_RGB_ARRAY_CONTIGUOUS,
                    output=rgb_output,
                )
                gray = env.observation(
                    OBSERVATION_MODE_GRAYSCALE,
                    output=gray_output,
                )

                self.assertIs(rgb_output, rgb)
                self.assertIs(gray_output, gray)
                self.assert_contiguous_uint8(rgb, SCREEN_SHAPE_24_BIT)
                self.assert_contiguous_uint8(gray, SCREEN_SHAPE_GRAYSCALE)
                self.assertTrue(np.array_equal(expected_rgb, rgb))
                self.assertTrue(np.array_equal(expected_gray, gray))
        finally:
            env.close()

    def test_copy_modes_reset_render_and_close_behavior(self):
        env = NESEnv(
            rom_file_abs_path('super-mario-bros-1.nes'),
            render_mode='rgb_array',
        )
        self.reset_frame(env, seed=29)
        rgb = env.observation(OBSERVATION_MODE_RGB_ARRAY_CONTIGUOUS)
        gray = env.observation(OBSERVATION_MODE_GRAYSCALE)
        self.assert_contiguous_uint8(rgb, SCREEN_SHAPE_24_BIT)
        self.assert_contiguous_uint8(gray, SCREEN_SHAPE_GRAYSCALE)
        self.assertIs(env.screen, env.render())

        env.step(0)
        stepped_rgb = env.observation(OBSERVATION_MODE_RGB_ARRAY_CONTIGUOUS)
        self.assertTrue(np.array_equal(np.ascontiguousarray(env.screen),
                                       stepped_rgb))

        env.reset(seed=29)
        reset_rgb = env.observation(OBSERVATION_MODE_RGB_ARRAY_CONTIGUOUS)
        self.assertTrue(np.array_equal(np.ascontiguousarray(env.screen),
                                       reset_rgb))

        env.close()
        self.assert_contiguous_uint8(rgb, SCREEN_SHAPE_24_BIT)
        self.assert_contiguous_uint8(gray, SCREEN_SHAPE_GRAYSCALE)
        self.assertIsInstance(int(rgb[0, 0, 0]), int)
        self.assertIsInstance(int(gray[0, 0]), int)
        self.assertIs(env.screen, env.observation())
        with self.assertRaises(ValueError):
            env.observation(OBSERVATION_MODE_RGB_ARRAY_CONTIGUOUS)
        with self.assertRaises(ValueError):
            env.observation(OBSERVATION_MODE_GRAYSCALE)

    def test_copy_modes_validate_output_and_mode(self):
        env = create_smb1_instance()
        try:
            self.reset_frame(env)
            with self.assertRaises(ValueError):
                env.observation(
                    OBSERVATION_MODE_RGB_ARRAY_CONTIGUOUS,
                    output=np.empty(SCREEN_SHAPE_GRAYSCALE, dtype=np.uint8),
                )
            with self.assertRaises(TypeError):
                env.observation(
                    OBSERVATION_MODE_GRAYSCALE,
                    output=np.empty(SCREEN_SHAPE_GRAYSCALE, dtype=np.uint16),
                )
            with self.assertRaises(ValueError):
                env.observation(
                    OBSERVATION_MODE_RGB_ARRAY_CONTIGUOUS,
                    output=np.empty(SCREEN_SHAPE_24_BIT,
                                    dtype=np.uint8,
                                    order='F'),
                )
            with self.assertRaises(NotImplementedError):
                env.observation('nearest_neighbor')
        finally:
            env.close()


class ShouldReadAndWriteMemory(TestCase):
    def test(self):
        env = create_smb1_instance()
        try:
            env.reset()
            for _ in range(90):
                env.step(8)
                env.step(0)
            self.assertEqual(129, env.ram[0x0776])
            env.ram[0x0776] = 0
            self.assertEqual(0, env.ram[0x0776])
        finally:
            env.close()


class ShouldResetAndCloseEnv(TestCase):
    def test(self):
        env = create_smb1_instance()
        env.reset()
        env.close()
        self.assertRaises(ValueError, env.close)


class ShouldStepEnv(NESEnvApplicationAssertions):
    def test(self):
        env = NESEnv(
            rom_file_abs_path('super-mario-bros-1.nes'),
            render_mode='rgb_array',
        )
        try:
            self.reset_frame(env, seed=23)
            for action in DETERMINISTIC_ACTIONS * 8:
                self.step_frame(env, action)
                self.assert_valid_frame(env.render())
            self.reset_frame(env)
        finally:
            env.close()


class ShouldUseGymnasiumLifecycle(NESEnvApplicationAssertions):
    """Focused tests for Gymnasium reset, step, seeding, and rendering."""

    def test_reset_seed_reinitializes_gymnasium_rng(self):
        env = create_smb1_instance()
        try:
            self.reset_frame(env, seed=123)
            first = int(env.np_random.integers(0, 1_000_000))
            self.reset_frame(env, seed=123)
            second = int(env.np_random.integers(0, 1_000_000))
            self.assertEqual(first, second)
        finally:
            env.close()

    def test_step_after_termination_requires_reset(self):
        env = TerminatingNESEnv(rom_file_abs_path('super-mario-bros-1.nes'))
        try:
            self.reset_frame(env)
            _, _, terminated, truncated, _ = self.step_frame(env, 0)
            self.assertTrue(terminated)
            self.assertFalse(truncated)
            with self.assertRaises(ValueError):
                env.step(0)
            self.reset_frame(env)
            self.step_frame(env, 0)
        finally:
            env.close()

    def test_legacy_done_hook_bridges_to_termination(self):
        env = LegacyDoneNESEnv(rom_file_abs_path('super-mario-bros-1.nes'))
        try:
            self.reset_frame(env)
            _, _, terminated, truncated, _ = self.step_frame(env, 0)
            self.assertTrue(terminated)
            self.assertFalse(truncated)
        finally:
            env.close()

    def test_render_without_render_mode_returns_none(self):
        env = create_smb1_instance()
        try:
            self.reset_frame(env)
            self.assertIsNone(env.render())
        finally:
            env.close()

    def test_rgb_array_render_returns_screen_without_viewer(self):
        env = NESEnv(
            rom_file_abs_path('super-mario-bros-1.nes'),
            render_mode='rgb_array',
        )
        try:
            self.reset_frame(env)
            with mock.patch('nes_py.nes_env.ImageViewer') as image_viewer:
                render = env.render()
            image_viewer.assert_not_called()
            self.assertIs(render, env.screen)
            self.assert_valid_frame(render)
        finally:
            env.close()

    def test_human_render_opens_and_reuses_viewer(self):
        env = NESEnv(
            rom_file_abs_path('super-mario-bros-1.nes'),
            render_mode='human',
        )
        try:
            self.reset_frame(env)
            with mock.patch('nes_py.nes_env.ImageViewer') as image_viewer:
                viewer = image_viewer.return_value
                self.assertIsNone(env.render())
                self.assertIsNone(env.render())
            image_viewer.assert_called_once()
            self.assertEqual(2, viewer.show.call_count)
            viewer.show.assert_called_with(env.screen)
        finally:
            env.close()


class ShouldPreservePackageBackupRestoreWorkflow(NESEnvApplicationAssertions):
    """
    Cover private backup/restore because speedtest and legacy wrappers still
    rely on this package-level workflow until a public replacement exists.
    """

    def test_backup_restore_returns_screen_to_backup_state(self):
        done = True
        env = create_smb1_instance()
        try:
            for _ in range(250):
                if done:
                    state, _ = env.reset()
                    done = False
                state, _, terminated, truncated, _ = env.step(0)
                done = terminated or truncated

            backup = state.copy()
            env._backup()

            for _ in range(250):
                if done:
                    state, _ = env.reset()
                    done = False
                state, _, terminated, truncated, _ = env.step(0)
                done = terminated or truncated

            self.assertFalse(np.array_equal(backup, state))
            env._restore()
            self.assertTrue(np.array_equal(backup, env.screen))
        finally:
            env.close()

    def test_restore_returns_screen_and_ram_to_backup_state(self):
        env = create_smb1_instance()
        try:
            self.reset_frame(env, seed=7)
            self.advance(env, [0, 8, 0, 8, 1, 2, 0, 4] * 6)

            backup_screen = env.screen.copy()
            backup_ram = env.ram.copy()
            env._backup()

            self.advance(env, [255, 0, 64, 128, 32, 16, 8, 4] * 6)
            screen_changed = not np.array_equal(backup_screen, env.screen)
            ram_changed = not np.array_equal(backup_ram, env.ram)
            self.assertTrue(screen_changed or ram_changed)

            env._restore()
            self.assertTrue(np.array_equal(backup_screen, env.screen))
            self.assertTrue(np.array_equal(backup_ram, env.ram))
            self.assert_valid_frame(env.screen)
        finally:
            env.close()

    def test_restored_continuation_matches_original_continuation(self):
        env = create_smb1_instance()
        try:
            self.reset_frame(env, seed=11)
            self.advance(env, [0, 8, 0, 8, 1, 2, 0, 4] * 5)
            env._backup()

            actions = [0, 1, 2, 4, 8, 16, 32, 64, 128, 255] * 3
            expected = self.advance(env, actions)
            env._restore()
            actual = self.advance(env, actions)

            for expected_output, actual_output in zip(expected, actual):
                (
                    expected_state,
                    expected_reward,
                    expected_terminated,
                    expected_truncated,
                    expected_info,
                ) = expected_output
                (
                    actual_state,
                    actual_reward,
                    actual_terminated,
                    actual_truncated,
                    actual_info,
                ) = actual_output
                self.assertTrue(np.array_equal(expected_state, actual_state))
                self.assertEqual(expected_reward, actual_reward)
                self.assertEqual(expected_terminated, actual_terminated)
                self.assertEqual(expected_truncated, actual_truncated)
                self.assertEqual(expected_info, actual_info)
        finally:
            env.close()

    def test_repeated_backup_restore_reset_cycles_keep_frames_valid(self):
        env = create_smb1_instance()
        try:
            state, _ = self.reset_frame(env, seed=13)
            env._backup()
            backup_screen = env.screen.copy()
            backup_ram = env.ram.copy()

            for step in range(1, 91):
                state, _, terminated, truncated, _ = env.step(
                    (step * 17) % env.action_space.n
                )
                done = terminated or truncated
                self.assert_valid_frame(state)

                if step % 7 == 0:
                    env._backup()
                    backup_screen = env.screen.copy()
                    backup_ram = env.ram.copy()

                if step % 11 == 0:
                    env._restore()
                    self.assertTrue(np.array_equal(backup_screen, env.screen))
                    self.assertTrue(np.array_equal(backup_ram, env.ram))
                    self.assert_valid_frame(env.screen)

                if step % 13 == 0:
                    state, _ = env.reset()
                    self.assertFalse(done)
                    self.assertTrue(np.array_equal(backup_screen, state))
                    self.assertTrue(np.array_equal(backup_ram, env.ram))
                    self.assert_valid_frame(state)
        finally:
            env.close()


class ShouldExposeOpaqueStateSnapshots(NESEnvApplicationAssertions):
    """Exercise the public opaque snapshot API across mapper fixtures."""

    def test_invalid_snapshot_input_raises_clear_error(self):
        env = create_smb1_instance()
        try:
            self.reset_frame(env, seed=41)
            with self.assertRaises(TypeError):
                env.load_state(object())
        finally:
            env.close()

    def test_snapshot_round_trip_restores_state_and_continuation(self):
        setup_actions = [0, 8, 0, 8, 1, 2, 0, 4]
        continuation_actions = [0, 1, 2, 4, 8, 16, 32, 64, 128, 255]
        for name in SNAPSHOT_MAPPER_ROM_NAMES:
            with self.subTest(name=name):
                env = NESEnv(rom_file_abs_path(name))
                try:
                    self.reset_frame(env, seed=43)
                    self.advance(env, setup_actions)
                    snapshot = env.dump_state()
                    snapshot_screen = env.screen.copy()
                    snapshot_ram = env.ram.copy()

                    expected = self.advance(env, continuation_actions)
                    env.load_state(snapshot)
                    self.assertTrue(np.array_equal(snapshot_screen,
                                                   env.screen))
                    self.assertTrue(np.array_equal(snapshot_ram, env.ram))

                    actual = self.advance(env, continuation_actions)
                    for expected_output, actual_output in zip(expected, actual):
                        self.assertTrue(np.array_equal(
                            expected_output[0],
                            actual_output[0],
                        ))
                        self.assertEqual(expected_output[1:],
                                         actual_output[1:])
                finally:
                    env.close()

    def test_snapshot_after_close_raises_but_existing_snapshot_remains_opaque(self):
        env = create_smb1_instance()
        self.reset_frame(env, seed=47)
        snapshot = env.dump_state()
        env.close()

        self.assertIsNotNone(snapshot)
        with self.assertRaises(ValueError):
            env.dump_state()
        with self.assertRaises(ValueError):
            env.load_state(snapshot)
