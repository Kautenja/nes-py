"""A gym wrapper for caching rewards."""
import gym


class RewardCacheEnv(gym.Wrapper):
    """a wrapper that caches rewards of episodes."""

    def __init__(self, env):
        """
        Initialize a reward caching environment.

        Args:
            env (gym.Env): the environment to wrap

        """
        super(RewardCacheEnv, self).__init__(env)
        self._score = 0
        self.env.unwrapped.episode_rewards = []

    def step(self, action):
        """
        Take a step using the given action.

        Args:
            action (any): the discrete action to perform.

        Returns:
            a tuple of:
            - (numpy.ndarray) the state as a result of the action
            - (float) the reward achieved by taking the action
            - (bool) a flag denoting whether the episode has ended
            - (dict) a dictionary of extra information

        """
        state, reward, done, info = self.env.step(action)
        self._score += reward
        if done:
            self.env.unwrapped.episode_rewards.append(self._score)
            self._score = 0
        return state, reward, done, info

    def reset(self):
        """Reset the emulator and return the initial state."""
        return self.env.reset()


# explicitly specify the external API of this module
__all__ = [RewardCacheEnv.__name__]
