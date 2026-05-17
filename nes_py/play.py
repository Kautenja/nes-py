"""Command line play helpers for the nes-py NES emulator."""
import argparse
import sys
import time

from tqdm import tqdm

from ._image_viewer import ImageViewer
from .nes_env import NESEnv


_NOP = 0


def _reset(env):
    """Reset a Gymnasium environment and return the observation."""
    observation, _ = env.reset()
    return observation


def _step(env, action):
    """Step a Gymnasium environment and return a combined done flag."""
    observation, reward, terminated, truncated, info = env.step(action)
    done = terminated or truncated
    return observation, reward, bool(done), info


def _keys_to_action(env):
    """Return the keyboard mapping for an environment."""
    if hasattr(env, 'get_keys_to_action'):
        return env.get_keys_to_action()
    if hasattr(env.unwrapped, 'get_keys_to_action'):
        return env.unwrapped.get_keys_to_action()
    raise ValueError('env has no get_keys_to_action method')


def _validate_pixel_observation_space(env):
    """Validate that an environment exposes image observations."""
    obs_s = env.observation_space
    is_bw = len(obs_s.shape) == 2
    is_rgb = len(obs_s.shape) == 3 and obs_s.shape[2] in [1, 3]
    if not (is_bw or is_rgb):
        raise ValueError('env observation_space must be image-like')


def _viewer_caption(env):
    """Return a readable caption for the play window."""
    if env.spec is not None:
        return env.spec.id
    return env.__class__.__name__


def _make_keyboard_viewer(env, keys_to_action):
    """Create an image viewer that also tracks relevant keyboard keys."""
    relevant_keys = {
        key for key_combo in keys_to_action.keys() for key in key_combo
    }
    return ImageViewer(
        _viewer_caption(env),
        env.observation_space.shape[0],
        env.observation_space.shape[1],
        monitor_keyboard=True,
        relevant_keys=relevant_keys,
    )


def play_human(env, callback=None):
    """
    Play the environment graphically using the keyboard.

    Args:
        env: the initialized Gymnasium environment to play
        callback: a callback to receive output from the environment

    Returns:
        None
    """
    _validate_pixel_observation_space(env)
    keys_to_action = _keys_to_action(env)
    viewer = _make_keyboard_viewer(env, keys_to_action)

    # Import pyglet after ImageViewer validates main-thread usage so parent
    # processes can import nes_py before spawning render-capable children.
    from pyglet import clock

    done = True
    target_frame_duration = 1 / env.metadata['render_fps']
    last_frame_time = 0

    try:
        while True:
            current_frame_time = time.time()
            if last_frame_time + target_frame_duration > current_frame_time:
                continue

            last_frame_time = current_frame_time
            clock.tick()

            if done:
                done = False
                state = _reset(env)
                viewer.show(env.unwrapped.screen)

            action = keys_to_action.get(viewer.pressed_keys, _NOP)
            next_state, reward, done, _ = _step(env, action)
            viewer.show(env.unwrapped.screen)

            if callback is not None:
                callback(state, action, reward, done, next_state)

            state = next_state

            if viewer.is_escape_pressed:
                break
    except KeyboardInterrupt:
        pass
    finally:
        viewer.close()
        env.close()


def play_random(env, steps, render=True, progress=True):
    """
    Play the environment by sampling uniformly random actions.

    Args:
        env: the initialized Gymnasium environment to play
        steps: the number of random steps to take
        render: whether to render frames to a graphical window
        progress: whether to display a progress bar

    Returns:
        None
    """
    try:
        done = True
        step_numbers = range(steps)
        if progress:
            step_numbers = tqdm(step_numbers)

        for _ in step_numbers:
            if done:
                _reset(env)
                done = False

            action = env.action_space.sample()
            _, reward, done, info = _step(env, action)

            if progress:
                step_numbers.set_postfix(reward=reward, info=info)
            if render:
                env.render()
    except KeyboardInterrupt:
        pass
    finally:
        env.close()


def _parser():
    """Build the command line parser."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        '--rom',
        '-r',
        required=True,
        help='path to the .nes ROM to play',
    )
    parser.add_argument(
        '--mode',
        '-m',
        choices=('human', 'random'),
        default='human',
        help='control mode for the emulator',
    )
    parser.add_argument(
        '--steps',
        '-s',
        type=int,
        default=500,
        help='number of random steps to take in random mode',
    )
    parser.add_argument(
        '--render',
        action=argparse.BooleanOptionalAction,
        default=True,
        help='render frames to a graphical window',
    )
    parser.add_argument(
        '--no-progress',
        action='store_false',
        dest='progress',
        help='disable the random-mode progress bar',
    )
    parser.set_defaults(progress=True)
    return parser


def main(argv=None):
    """Run the command line play interface."""
    parser = _parser()
    args = parser.parse_args(argv)

    if args.mode == 'human' and not args.render:
        parser.error('human mode requires graphical rendering')
    if args.mode == 'random' and args.steps <= 0:
        parser.error('--steps must be positive in random mode')

    render_mode = 'human' if args.mode == 'random' and args.render else None
    env = NESEnv(args.rom, render_mode=render_mode)
    if args.mode == 'human':
        play_human(env)
    else:
        play_random(
            env,
            args.steps,
            render=args.render,
            progress=args.progress,
        )
    return 0


__all__ = [
    main.__name__,
    play_human.__name__,
    play_random.__name__,
]


if __name__ == '__main__':
    sys.exit(main())
