"""A gym wrapper for penalizing deaths."""
import gym


class PenalizeDeathEnv(gym.Wrapper):
    """a wrapper that penalizes deaths, without terminating episodes."""

    def __init__(self, env, penalty=-15):
        """
        Initialize a new death penalizing environment wrapper.

        Args:
            env (gym.Env): the environment to wrap
            penalty (float): the penalty for losing a life

        Returns:
            None

        """
        super(PenalizeDeathEnv, self).__init__(env)
        self._penalty = penalty

    def step(self, action):
        """
        Take a step using the given action.

        Args:
            action (any): the discrete action to perform

        Returns:
            a tuple of:
            - (numpy.ndarray) the state as a result of the action
            - (float) the reward achieved by taking the action
            - (bool) a flag denoting whether the episode has ended
            - (dict) a dictionary of extra information

        """
        obs, reward, done, info = self.env.step(action)
        # update the reward based on the done flag
        reward = self._penalty if done else reward

        return obs, reward, done, info

    def reset(self):
        """Reset the emulator and return the initial state."""
        return self.env.reset()


# explicitly specify the external API of this module
__all__ = [PenalizeDeathEnv.__name__]
