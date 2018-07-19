"""Test cases for the NESEnv class."""
from unittest import TestCase


class ShouldImportNESEnv(TestCase):
    def test(self):
        try:
            from ..nes_env import NESEnv
        except ImportError:
            self.fail('failed to import NESEnv from nes_py.nes_env')


class ShouldRaiseTypeErrorOnInvalidROMPathType(TestCase):
    def test(self):
        from ..nes_env import NESEnv
        self.assertRaises(TypeError, NESEnv, 0)


class ShouldRaiseValueErrorOnMissingNonexistentROMFile(TestCase):
    def test(self):
        import os
        from ..nes_env import NESEnv
        path =  os.path.join(os.path.dirname(__file__), 'games/missing.nes')
        self.assertRaises(ValueError, NESEnv, path)


class ShouldRaiseValueErrorOnNoniNES_ROMPath(TestCase):
    def test(self):
        import os
        from ..nes_env import NESEnv
        path =  os.path.join(os.path.dirname(__file__), 'games/blank')
        self.assertRaises(ValueError, NESEnv, 'not_a_file.nes')


class ShouldRaiseValueErrorOnInvalidiNES_ROMPath(TestCase):
    def test(self):
        import os
        from ..nes_env import NESEnv
        path =  os.path.join(os.path.dirname(__file__), 'games/empty.nes')
        self.assertRaises(ValueError, NESEnv, path)


class ShouldCreateInstanceOfNESEnv(TestCase):
    def test(self):
        import os
        from ..nes_env import NESEnv
        import gym
        path =  os.path.join(os.path.dirname(__file__), 'games/smb1.nes')
        self.assertIsInstance(NESEnv(path), gym.Env)


def create_smb1_instance():
        import os
        from ..nes_env import NESEnv
        import gym
        path =  os.path.join(os.path.dirname(__file__), 'games/smb1.nes')
        return NESEnv(path)


class ShouldResetAndCloseEnv(TestCase):
    def test(self):
        env = create_smb1_instance()
        env.reset()
        env.close()
        # trying to close again should raise an error
        self.assertRaises(ValueError, env.close)


class ShouldStepEnv(TestCase):
    def test(self):
        import numpy as np
        env = create_smb1_instance()
        done = False
        for i in range(500):
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
            self.assertIsInstance(reward, int)
            self.assertIsInstance(done, bool)
            self.assertIsInstance(info, dict)
            # check the render output
            render = env.render('rgb_array')
            self.assertIsInstance(render, np.ndarray)
        env.close()
