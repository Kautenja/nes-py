"""NES emulator for OpenAI Gym."""
import argparse
from .play import play_human, play_random
from ..nes_env import NesENV


def create_argparser():
    """Create and return an argument parser for this command line interface."""
    parser = argparse.ArgumentParser(description=__doc__)
    # add the argument for the Super Mario Bros environment to run
    parser.add_argument('--rom', '-r',
        type=str,
        help='The path to the ROM to play.'
    )
    # add the argument for the mode of execution as either human or random
    parser.add_argument('--mode', '-m',
        type=str,
        default='human',
        choices=['human', 'random'],
        help='The execution mode for the emulation.'
    )

    return parser


def main():
    """The main entry point for the command line interface."""
    # parse arguments from the command line (argparse validates arguments)
    args = create_argparser().parse_args()
    # select the method for playing the game
    if args.mode == 'human':
        play = play_human
    elif args.mode == 'random':
        play = play_random
    # play the game
    env = NesENV(args.rom)
    play(env)


# explicitly define the outward facing API of this module
__all__ = [main.__name__]
