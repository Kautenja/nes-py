"""Command line interface to nes-py NES emulator."""
import argparse
from .play import play_human, play_random
from ..nes_env import NESEnv


# The play modes for the system
_PLAY_MODES = {
    'human': play_human,
    'random': play_random
}


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

    return parser.parse_args()


def main():
    """The main entry point for the command line interface."""
    # get arguments from the command line
    args = _get_args()
    # create the environment
    env = NESEnv(args.rom)
    # play the environment with the given mode
    _PLAY_MODES[args.mode](env)


# explicitly define the outward facing API of this module
__all__ = [main.__name__]
