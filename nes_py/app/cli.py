"""Command line interface to nes-py NES emulator."""
import argparse
from .play_human import play_human
from .play_random import play_random
from ..nes_env import NESEnv


def _get_args():
    """Parse arguments from the command line and return them."""
    parser = argparse.ArgumentParser(description=__doc__)
    # add the argument for the Super Mario Bros environment to run
    parser.add_argument('--rom', '-r',
        type=str,
        help='The path to the ROM to play.',
        required=True,
    )
    # add the argument for the mode of execution as either human or random
    parser.add_argument('--mode', '-m',
        type=str,
        default='human',
        choices=['human', 'random'],
        help='The execution mode for the emulation.',
    )
    parser.add_argument('--seed', '-S',
        type=int,
        help='the random number seed to use'
    )
    # add the argument for the number of steps to take in random mode
    parser.add_argument('--steps', '-s',
        type=int,
        default=500,
        help='The number of random steps to take.',
    )
    return parser.parse_args()


def main():
    """The main entry point for the command line interface."""
    # get arguments from the command line
    args = _get_args()
    # play the environment with the given mode
    if args.mode == 'human':
        # environment is initialized without a rendering mode, as play_human creates its own
        env = NESEnv(args.rom)
        play_human(env, seed=args.seed)
    else:
        # create the environment
        env = NESEnv(args.rom, render_mode='human')
        play_random(env, args.steps, seed=args.seed)


# explicitly define the outward facing API of this module
__all__ = [main.__name__]
