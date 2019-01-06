"""Wrappers for altering the functionality of the game."""
from .binary_to_discrete_space_env import BinarySpaceToDiscreteSpaceEnv
from .clip_reward_env import ClipRewardEnv
from .downsample_env import DownsampleEnv
from .frame_skip_env import FrameSkipEnv
from .frame_stack_env import FrameStackEnv
from .normalize_reward_env import NormalizeRewardEnv
from .penalize_death_env import PenalizeDeathEnv
from .reward_cache_env import RewardCacheEnv


def wrap(env,
    frame_skip=4,
    cache_rewards=True,
    image_size=(84, 84),
    death_penalty=-15,
    clip_rewards=False,
    normalize_rewards=False,
    agent_history_length=4
):
    """
    Wrap an environment with standard wrappers.

    Args:
        env (gym.Env): the environment to wrap
        frame_skip (int): the number of frames to skip between observations
        cache_rewards (bool): True to use a reward cache for raw rewards
        image_size (tuple): the size to down-sample images to
        death_penatly (float): the penalty for losing a life in a game
        clip_rewards (bool): whether to clip rewards in {-1, 0, +1}
        normalize_rewards (bool): whether to normalize rewards w/ infinity norm
        agent_history_length (int): the size of the frame buffer for the agent

    Returns:
        a gym environment configured for this experiment

    """
    # wrap the environment with a frame skipper
    if frame_skip:
        env = FrameSkipEnv(env, frame_skip)
    # wrap the environment with a reward cacher
    if cache_rewards:
        env = RewardCacheEnv(env)
    # apply a down-sampler for the given game
    if image_size is not None:
        env = DownsampleEnv(env, image_size)
    # apply the death penalty feature if enabled
    if death_penalty is not None:
        env = PenalizeDeathEnv(env, penalty=death_penalty)
    # normalize the rewards in [-1, 1] if the feature is enabled
    if normalize_rewards:
        env = NormalizeRewardEnv(env)
    # clip the rewards in {-1, 0, +1} if the feature is enabled
    if clip_rewards:
        env = ClipRewardEnv(env)
    # apply the back history of frames if the feature is enabled
    if agent_history_length is not None:
        env = FrameStackEnv(env, agent_history_length)

    return env


# explicitly define the outward facing API of this package
__all__ = [
    BinarySpaceToDiscreteSpaceEnv.__name__,
    ClipRewardEnv.__name__,
    DownsampleEnv.__name__,
    FrameSkipEnv.__name__,
    FrameStackEnv.__name__,
    NormalizeRewardEnv.__name__,
    PenalizeDeathEnv.__name__,
    RewardCacheEnv.__name__,
    wrap.__name__,
]
