<p align="center">
<img
    src="https://user-images.githubusercontent.com/2184469/42918029-a8364c66-8ad1-11e8-8147-2653091ccd38.png"
    width="50%"
/>
</p>

[![build-status][]][ci-server]
[![PackageVersion][pypi-version]][pypi-home]
[![PythonVersion][python-version]][python-home]
[![Stable][pypi-status]][pypi-home]
[![Format][pypi-format]][pypi-home]
[![License][pypi-license]](LICENSE)

[build-status]: https://travis-ci.com/Kautenja/nes-py.svg?token=FCkX2qMNHzx2qWEzZZMP&branch=master
[ci-server]: https://travis-ci.com/Kautenja/nes-py
[pypi-version]: https://badge.fury.io/py/nes-py.svg
[pypi-license]: https://img.shields.io/pypi/l/nes-py.svg
[pypi-status]: https://img.shields.io/pypi/status/nes-py.svg
[pypi-format]: https://img.shields.io/pypi/format/nes-py.svg
[pypi-home]: https://badge.fury.io/py/nes-py
[python-version]: https://img.shields.io/pypi/pyversions/nes-py.svg
[python-home]: https://python.org

nes-py is an NES emulator and OpenAI Gym interface based on the
[LaiNES](https://github.com/AndreaOrru/LaiNES) emulator.

# Installation

The preferred installation of `nes-py` is from `pip`:

```shell
pip install nes-py
```

# Usage

To access the NES emulator from the command line use the following command.

```shell
nes_py -r <path_to_rom>
```

To print out documentation for the command line interface execute:

```shell
nes_py -h
```

## Controls

| Keyboard Key | NES Joypad |
|:-------------|:-----------|
| W            | Up         |
| A            | Left       |
| S            | Down       |
| D            | Right      |
| O            | A          |
| P            | B          |
| Enter        | Start      |
| Space        | Select     |

# Development

To design a custom environment using `nes-py`, introduce new features, or fix
a bug, please refer to the [Wiki](https://github.com/Kautenja/nes-py/wiki).
There you will find instructions for:

-   setting up the development environment
-   designing environments based on the `NESEnv` class
-   reference material for the `NESEnv` API

# Compatibility

nes-py implements the most common mappers, which should be enough for a good
percentage of the games:

-   NROM (Mapper 000)
-   MMC1 / SxROM (Mapper 001)
-   UxROM (Mapper 002)
-   CNROM (Mapper 003)
-   MMC3, MMC6 / TxROM (Mapper 004)

You can check the compatibility for each ROM in the following
[list](https://github.com/Kautenja/nes-py/blob/master/nesmapper.txt)
