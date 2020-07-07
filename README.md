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

[build-status]: https://travis-ci.org/Kautenja/nes-py.svg
[ci-server]: https://travis-ci.org/Kautenja/nes-py
[pypi-version]: https://badge.fury.io/py/nes-py.svg
[pypi-license]: https://img.shields.io/pypi/l/nes-py.svg
[pypi-status]: https://img.shields.io/pypi/status/nes-py.svg
[pypi-format]: https://img.shields.io/pypi/format/nes-py.svg
[pypi-home]: https://badge.fury.io/py/nes-py
[python-version]: https://img.shields.io/pypi/pyversions/nes-py.svg
[python-home]: https://python.org

nes-py is an NES emulator and OpenAI Gym interface for MacOS, Linux, and
Windows based on the [SimpleNES](https://github.com/amhndu/SimpleNES) emulator.

<table align="center">
    <tr>
        <td>
            <img
                width="256"
                alt="Bomberman II"
                src="https://user-images.githubusercontent.com/2184469/84821320-8c52e780-afe0-11ea-820a-662d0e54fc90.png"
            />
        </td>
        <td>
             <img
                width="256"
                alt="Castelvania II"
                src="https://user-images.githubusercontent.com/2184469/84821323-8ceb7e00-afe0-11ea-89f1-56d379ae4286.png"
            />
        </td>
        <td>
            <img
                width="256"
                alt="Excitebike"
                src="https://user-images.githubusercontent.com/2184469/84821325-8d841480-afe0-11ea-9ae2-599b83af6f65.png"
            />
        </td>
    </tr>
    <tr>
        <td>
            <img
                width="256"
                alt="Super Mario Bros."
                src="https://user-images.githubusercontent.com/2184469/84821327-8d841480-afe0-11ea-8172-d564aca35b5e.png"
            />
        </td>
        <td>
            <img
                width="256"
                alt="The Legend of Zelda"
                src="https://user-images.githubusercontent.com/2184469/84821329-8d841480-afe0-11ea-9a57-c9daca04ed3b.png"
            />
        </td>
        <td>
            <img
                 width="256"
                 alt="Tetris"
                 src="https://user-images.githubusercontent.com/2184469/84822244-fc15a200-afe1-11ea-81de-2323845d7537.png"
            />
        </td>
    </tr>
    <tr>
        <td>
            <img
                 width="256"
                 alt="Contra"
                 src="https://user-images.githubusercontent.com/2184469/84822247-fcae3880-afe1-11ea-901d-1ef5e8378989.png"
            />
        </td>
        <td>
            <img
                 width="256"
                 alt="Mega Man II"
                 src="https://user-images.githubusercontent.com/2184469/84822249-fcae3880-afe1-11ea-8271-9e898933e571.png"
            />
        </td>
        <td>
            <img
                width="256"
                alt="Bubble Bobble"
                src="https://user-images.githubusercontent.com/2184469/84822551-79411700-afe2-11ea-9ed6-947d78f29e8f.png"
            />
        </td>
    </tr>
</table>

# Installation

The preferred installation of `nes-py` is from `pip`:

```shell
pip install nes-py
```

## Debian

Make sure you have the `clang++` compiler installed:

```shell
sudo apt-get install clang
```

## Windows

You'll need to install the Visual-Studio 17.0 tools for Windows installation.
The [Visual Studio Community](https://visualstudio.microsoft.com/downloads/)
package provides these tools for free.

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

| Keyboard Key | NES Joypad    |
|:-------------|:--------------|
| W            | Up            |
| A            | Left          |
| S            | Down          |
| D            | Right         |
| O            | A             |
| P            | B             |
| Enter        | Start         |
| Space        | Select        |

## Parallelism Caveats

both the `threading` and `multiprocessing` packages are supported by
`nes-py` with some caveats related to rendering:

1.  rendering **is not** supported from instances of `threading.Thread`
2.  rendering **is** supported from instances of `multiprocessing.Process`,
    but `nes-py` must be imported within the process that executes the render
    call

# Development

To design a custom environment using `nes-py`, introduce new features, or fix
a bug, please refer to the [Wiki](https://github.com/Kautenja/nes-py/wiki).
There you will find instructions for:

-   setting up the development environment
-   designing environments based on the `NESEnv` class
-   reference material for the `NESEnv` API
-   documentation for the `nes_py.wrappers` module

# Cartridge Mapper Compatibility

0.  NROM
1.  MMC1 / SxROM
2.  UxROM
3.  CNROM

You can check the compatibility for each ROM in the following
[list](https://github.com/Kautenja/nes-py/blob/master/nesmapper.txt)

# Disclaimer

**This project is provided for educational purposes only. It is not
affiliated with and has not been approved by Nintendo.**
