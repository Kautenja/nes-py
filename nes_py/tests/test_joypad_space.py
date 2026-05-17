"""Tests for Gymnasium JoypadSpace wrapper behavior."""
from unittest import TestCase

import gymnasium as gym

from nes_py.wrappers import JoypadSpace


class RecordingEnv(gym.Env):
    """Small Gymnasium environment that records wrapper forwarding."""

    observation_space = gym.spaces.Discrete(2)
    action_space = gym.spaces.Discrete(256)

    def __init__(self):
        """Initialize recorded call state."""
        self.reset_seed = None
        self.reset_options = None
        self.step_action = None

    def reset(self, *, seed=None, options=None):
        """Record reset kwargs and return a Gymnasium reset tuple."""
        super().reset(seed=seed)
        self.reset_seed = seed
        self.reset_options = options
        return 0, {'seed': seed, 'options': options}

    def step(self, action):
        """Record the byte action and return a Gymnasium step tuple."""
        self.step_action = action
        return 1, 2.0, False, True, {'action': action}

    def get_keys_to_action(self):
        """Return key mappings for the test action byte values."""
        return {
            (): 0,
            (1, 2): 0b10000001,
            (3,): 0b01000110,
        }


class JoypadSpaceTest(TestCase):
    """Test the public JoypadSpace wrapper contract."""

    def test_reset_forwards_seed_options_and_metadata(self):
        env = RecordingEnv()
        wrapper = JoypadSpace(env, [['NOOP']])

        output = wrapper.reset(seed=7, options={'difficulty': 'test'})

        self.assertEqual((0, {
            'seed': 7,
            'options': {'difficulty': 'test'},
        }), output)
        self.assertEqual(7, env.reset_seed)
        self.assertEqual({'difficulty': 'test'}, env.reset_options)

    def test_step_maps_discrete_action_to_byte_and_keeps_five_tuple(self):
        env = RecordingEnv()
        wrapper = JoypadSpace(env, [
            ['NOOP'],
            ['right', 'A'],
            ['left', 'B', 'select'],
        ])

        output = wrapper.step(1)

        self.assertEqual(0b10000001, env.step_action)
        self.assertEqual((1, 2.0, False, True, {
            'action': 0b10000001,
        }), output)

    def test_action_metadata_uses_discrete_action_space(self):
        wrapper = JoypadSpace(RecordingEnv(), [
            ['NOOP'],
            ['right', 'A'],
            ['left', 'B', 'select'],
        ])

        self.assertEqual(3, wrapper.action_space.n)
        self.assertEqual([
            'NOOP',
            'right A',
            'left B select',
        ], wrapper.get_action_meanings())
        self.assertEqual({
            (): 0,
            (1, 2): 1,
            (3,): 2,
        }, wrapper.get_keys_to_action())
