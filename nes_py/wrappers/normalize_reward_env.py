"""An environment wrapper to normalize rewards."""
import gym


class NormalizeRewardEnv(gym.RewardWrapper):
    """An environment that normalizes rewards about an L infinity norm."""

    def __init__(self, env):
        """
        Initialize a new reward clipping environment.

        Args:
            env (gym.Env): the environment to wrap

        Returns:
            None

        """
        super(NormalizeRewardEnv, self).__init__(env)
        # setup the l infinity reward
        self._l_inf_norm = max([abs(r) for r in env.reward_range])
        # raise an error if the reward range is infinitely bounded
        if self._l_inf_norm == float('inf'):
            raise ValueError('cant normalize a reward with infinite range')

    def reward(self, reward):
        """
        Normalize the reward.

        Args:
            reward (float): the reward to normalize

        Returns:
            (float) the L infinity normalize reward

        """
        return reward / self._l_inf_norm


# explicitly specify the external API of this module
__all__ = [NormalizeRewardEnv.__name__]
