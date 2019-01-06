"""A gym wrapper for skipping frames."""
import gym


class FrameSkipEnv(gym.Wrapper):
    """a wrapper that skips frames deterministically."""

    def __init__(self, env, skip=4):
        """
        Initialize a new death penalizing environment wrapper.

        Args:
            env (gym.Env): the environment to wrap
            skip (int): the number of frames to skip

        Returns:
            None

        """
        super(FrameSkipEnv, self).__init__(env)
        self.skip = skip

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
        # create a placeholder for the total reward
        total_reward = 0.0
        # set the done flag to false
        done = False
        # iterate over the number of frames to skip
        for _ in range(self.skip):
            # get the next state using the given action
            state, reward, done, info = self.env.step(action)
            # accumulate the total reward
            total_reward += reward
            # if the episode ended, break out of the look
            if done:
                break

        return state, total_reward, done, info

    def reset(self):
        """Reset the emulator and return the initial state."""
        return self.env.reset()


# explicitly specify the external API of this module
__all__ = [FrameSkipEnv.__name__]
