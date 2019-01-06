"""A method to get absolute paths of game ROMs."""
import os


def rom_file_abs_path(file_name: str) -> str:
    """
    Return the absolute path to a ROM in the games directory.

    Args:
        file_name: the name of the ROM in the games directory to fetch

    Returns:
        the absolute path to the given ROM filename in the games directory

    """
    # the directory of this file
    dir_path = os.path.dirname(os.path.realpath(__file__))
    # the absolute path to the given ROM file
    game_path = '{}/games/{}'.format(dir_path, file_name)

    return game_path


# explicitly define the outward facing API of this module
__all__ = [rom_file_abs_path.__name__]
