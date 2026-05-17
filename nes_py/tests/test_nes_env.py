"""Test cases for the NESEnv class."""
from unittest import TestCase
import gym
import numpy as np
from .rom_file_abs_path import rom_file_abs_path
from nes_py.nes_env import NESEnv
from nes_py.nes_env import SCREEN_SHAPE_24_BIT


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
        env = NESEnv(rom_file_abs_path('super-mario-bros-1.nes'))
        self.assertRaises(ValueError, env.step, 0)


class ShouldCreateInstanceOfNESEnv(TestCase):
    def test(self):
        env = NESEnv(rom_file_abs_path('super-mario-bros-1.nes'))
        self.assertIsInstance(env, gym.Env)
        env.close()


def create_smb1_instance():
    """Return a new SMB1 instance."""
    return NESEnv(rom_file_abs_path('super-mario-bros-1.nes'))


class ShouldReadAndWriteMemory(TestCase):
    def test(self):
        env = create_smb1_instance()
        env.reset()
        for _ in range(90):
            env.step(8)
            env.step(0)
        self.assertEqual(129, env.ram[0x0776])
        env.ram[0x0776] = 0
        self.assertEqual(0, env.ram[0x0776])
        env.close()


class ShouldResetAndCloseEnv(TestCase):
    def test(self):
        env = create_smb1_instance()
        env.reset()
        env.close()
        # trying to close again should raise an error
        self.assertRaises(ValueError, env.close)


class ShouldStepEnv(TestCase):
    def test(self):
        env = create_smb1_instance()
        done = True
        for _ in range(500):
            if done:
                # reset the environment and check the output value
                state = env.reset()
                self.assertIsInstance(state, np.ndarray)
            # sample a random action and check it
            action = env.action_space.sample()
            self.assertIsInstance(action, int)
            # take a step and check the outputs
            output = env.step(action)
            self.assertIsInstance(output, tuple)
            self.assertEqual(4, len(output))
            # check each output
            state, reward, done, info = output
            self.assertIsInstance(state, np.ndarray)
            self.assertIsInstance(reward, float)
            self.assertIsInstance(done, bool)
            self.assertIsInstance(info, dict)
            # check the render output
            render = env.render('rgb_array')
            self.assertIsInstance(render, np.ndarray)
        env.reset()
        env.close()


class ShouldStepEnvBackupRestore(TestCase):
    def _assert_valid_frame(self, state):
        self.assertIsInstance(state, np.ndarray)
        self.assertEqual(SCREEN_SHAPE_24_BIT, state.shape)
        self.assertEqual(np.uint8, state.dtype)

    def _advance(self, env, actions):
        outputs = []
        for action in actions:
            state, reward, done, info = env.step(action)
            outputs.append((state.copy(), reward, done, info.copy()))
        return outputs

    def test(self):
        done = True
        env = create_smb1_instance()

        for _ in range(250):
            if done:
                state = env.reset()
                done = False
            state, _, done, _ = env.step(0)

        backup = state.copy()

        env._backup()

        for _ in range(250):
            if done:
                state = env.reset()
                done = False
            state, _, done, _ = env.step(0)

        self.assertFalse(np.array_equal(backup, state))
        env._restore()
        self.assertTrue(np.array_equal(backup, env.screen))
        env.close()

    def test_restore_returns_screen_and_ram_to_backup_state(self):
        env = create_smb1_instance()
        env.reset(seed=7)
        self._advance(env, [0, 8, 0, 8, 1, 2, 0, 4] * 6)

        backup_screen = env.screen.copy()
        backup_ram = env.ram.copy()
        env._backup()

        self._advance(env, [255, 0, 64, 128, 32, 16, 8, 4] * 6)
        screen_changed = not np.array_equal(backup_screen, env.screen)
        ram_changed = not np.array_equal(backup_ram, env.ram)
        self.assertTrue(screen_changed or ram_changed)

        env._restore()
        self.assertTrue(np.array_equal(backup_screen, env.screen))
        self.assertTrue(np.array_equal(backup_ram, env.ram))
        self._assert_valid_frame(env.screen)
        env.close()

    def test_restored_continuation_matches_original_continuation(self):
        env = create_smb1_instance()
        env.reset(seed=11)
        self._advance(env, [0, 8, 0, 8, 1, 2, 0, 4] * 5)
        env._backup()

        actions = [0, 1, 2, 4, 8, 16, 32, 64, 128, 255] * 3
        expected = self._advance(env, actions)
        env._restore()
        actual = self._advance(env, actions)

        for expected_output, actual_output in zip(expected, actual):
            expected_state, expected_reward, expected_done, expected_info = (
                expected_output
            )
            actual_state, actual_reward, actual_done, actual_info = (
                actual_output
            )
            self.assertTrue(np.array_equal(expected_state, actual_state))
            self.assertEqual(expected_reward, actual_reward)
            self.assertEqual(expected_done, actual_done)
            self.assertEqual(expected_info, actual_info)
        env.close()

    def test_repeated_backup_restore_reset_cycles_keep_frames_valid(self):
        env = create_smb1_instance()
        state = env.reset(seed=13)
        self._assert_valid_frame(state)
        env._backup()
        backup_screen = env.screen.copy()
        backup_ram = env.ram.copy()

        for step in range(1, 91):
            state, _, done, _ = env.step((step * 17) % env.action_space.n)
            self._assert_valid_frame(state)

            if step % 7 == 0:
                env._backup()
                backup_screen = env.screen.copy()
                backup_ram = env.ram.copy()

            if step % 11 == 0:
                env._restore()
                self.assertTrue(np.array_equal(backup_screen, env.screen))
                self.assertTrue(np.array_equal(backup_ram, env.ram))
                self._assert_valid_frame(env.screen)

            if step % 13 == 0:
                state = env.reset()
                self.assertFalse(done)
                self.assertTrue(np.array_equal(backup_screen, state))
                self.assertTrue(np.array_equal(backup_ram, env.ram))
                self._assert_valid_frame(state)

        env.close()
