"""Test cases for the NESEnv class."""
from unittest import TestCase
from PIL import Image


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
        env = NESEnv(path)
        self.assertIsInstance(env, gym.Env)
        env.close()


def create_smb1_instance():
    """Return a new SMB1 instance."""
    import os
    from ..nes_env import NESEnv
    import gym
    path = os.path.join(os.path.dirname(__file__), 'games/smb1.nes')
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
        done = True
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
        env.reset()
        env.close()


class ShouldStepEnvBackupRestore(TestCase):
    def test(self):
        import numpy as np
        done = True
        env = create_smb1_instance()

        for i in range(250):
            if done:
                state = env.reset()
                done = False
            state, _, done, _ = env.step(0)

        backup = state.copy()
        # Image.fromarray(backup).save('state.png')
        env._backup()

        for i in range(250):
            if done:
                state = env.reset()
                done = False
            state, _, done, _ = env.step(0)

        # Image.fromarray(state).save('state1.png')
        self.assertFalse(np.array_equal(backup, state))
        env._restore()
        self.assertTrue(np.array_equal(backup, env.screen))
        env.close()
