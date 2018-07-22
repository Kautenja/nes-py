"""An environment wrapper to clip rewards."""
import gym
import numpy as np


class ClipRewardEnv(gym.RewardWrapper):
    """An environment that clips rewards in {-1, 0, 1}."""

    def __init__(self, env):
        """
        Initialize a new reward clipping environment.

        Args:
            env (gym.Env): the environment to wrap

        Returns:
            None

        """
        super().__init__(env)

    def reward(self, reward):
        """
        Bin reward to {-1, 0, +1} using its sign.

        Args:
            reward (float): the reward to adjust

        Returns:
            an adjusted reward as the sign of the input reward. i.e.:
            -   -1 if reward < 0
            -   1 if reward > 0
            -   0 if reward == 0

        """
        return np.sign(reward)


# explicitly specify the external API of this module
__all__ = [ClipRewardEnv.__name__]
