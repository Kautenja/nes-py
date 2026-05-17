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

[build-status]: https://github.com/Kautenja/nes-py/actions/workflows/ci.yml/badge.svg?branch=master
[ci-server]: https://github.com/Kautenja/nes-py/actions/workflows/ci.yml
[pypi-version]: https://badge.fury.io/py/nes-py.svg
[pypi-license]: https://img.shields.io/pypi/l/nes-py.svg
[pypi-status]: https://img.shields.io/pypi/status/nes-py.svg
[pypi-format]: https://img.shields.io/pypi/format/nes-py.svg
[pypi-home]: https://badge.fury.io/py/nes-py
[python-version]: https://img.shields.io/pypi/pyversions/nes-py.svg
[python-home]: https://python.org

nes-py is an NES emulator and Gymnasium interface for MacOS, Linux, and
Windows based on the [SimpleNES](https://github.com/amhndu/SimpleNES) emulator.
The supported CI target is Python 3.13.

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
python3 -m nes_py.play --rom <path_to_rom>
```

To print out documentation for the command line interface execute:

```shell
python3 -m nes_py.play -h
```

The play command supports keyboard controls and random controls. Random play can
run with or without a graphical window:

```shell
python3 -m nes_py.play --rom <path_to_rom> --mode random --steps 500
python3 -m nes_py.play --rom <path_to_rom> --mode random --steps 500 --no-render
```

To use the Python API directly, construct the environment with the desired
Gymnasium render mode, seed through `reset`, and handle the separated
termination and truncation flags:

```python
from nes_py.nes_env import NESEnv

env = NESEnv("<path_to_rom>", render_mode="rgb_array")
observation, info = env.reset(seed=123)
terminated = False
truncated = False

while not (terminated or truncated):
    action = env.action_space.sample()
    observation, reward, terminated, truncated, info = env.step(action)
    frame = env.render()

env.close()
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

Both the `threading` and `multiprocessing` packages are supported by
`nes-py`. The rendering caveats only apply to windowed `human` rendering:

1.  `rgb_array` rendering is supported from `threading.Thread` and
    `multiprocessing.Process` instances.
2.  `human` rendering **is not** supported from instances of
    `threading.Thread`; it must run on the process's main Python thread.
3.  `human` rendering **is** supported from instances of
    `multiprocessing.Process`, but the viewer must be created in the process
    that owns the render call. Importing `nes-py` or `nes_py.play` in a parent
    process does not initialize the windowing backend.

# Development

To design a custom environment using `nes-py`, introduce new features, or fix
a bug, please refer to the [Wiki](https://github.com/Kautenja/nes-py/wiki).
There you will find instructions for:

-   setting up the development environment
-   designing environments based on the `NESEnv` class
-   reference material for the `NESEnv` API
-   documentation for the `nes_py.wrappers` module

Project metadata, runtime dependencies, release extras, console scripts, and
package discovery are configured in `pyproject.toml`. The native emulator
source tree lives under `nes_emu`, with public and internal headers below
`nes_emu/include/nes_emu` and C++ sources below `nes_emu/src/nes_emu`. CMake
builds those sources into the `nes_py._native` extension through
scikit-build-core. The runtime binding imports `nes_py._native` directly; the
old `ctypes` shared-library discovery path is no longer used. For local
development, install the package in editable mode and build distributions
through the standard PEP 517 frontend:

```shell
python -m pip install --upgrade pip build
python -m pip install --editable . --config-settings=editable.mode=inplace
python -m unittest discover .
./main.sh clean
python -m build
```

Native emulator internals are tested and benchmarked through opt-in CMake
targets so normal Python installs do not fetch test dependencies:

```shell
cmake -S . -B build/nes-emu-debug -DCMAKE_BUILD_TYPE=Debug -DNES_EMU_BUILD_TESTS=ON
cmake --build build/nes-emu-debug --target nes_emu_tests
ctest --test-dir build/nes-emu-debug --output-on-failure
cmake -S . -B build/nes-emu-release -DCMAKE_BUILD_TYPE=Release -DNES_EMU_BUILD_BENCHMARKS=ON
cmake --build build/nes-emu-release --target nes_emu_benchmarks
```

PyPI releases are published by the `Publish to PyPI` GitHub Actions workflow
through PyPI trusted publishing, not by local `twine` credentials. Configure the
PyPI project publisher with owner `Kautenja`, repository `nes-py`, workflow
filename `publish.yml`, and environment `pypi`. Then create a GitHub release
from a tag matching `pyproject.toml`'s version, with or without a leading `v`.
The workflow builds the source distribution and CPython 3.13 wheels for Linux,
Windows, and macOS before publishing.

## Benchmarking

Developer throughput checks are available through the packaged speedtest
module:

```shell
python -m nes_py.speedtest --rom nes_py/tests/games/super-mario-bros-1.nes --steps 5000
```

Use `--json` for machine-readable output. Benchmark numbers are informational
and vary by machine, compiler, runner load, and display settings; they are not
correctness criteria. Backup and restore stress options use explicit interval
semantics, so `--backup-interval 12` runs a backup at steps 12, 24, 36, and so
on.

# Cartridge Mapper Compatibility

0.  NROM
1.  MMC1 / SxROM
2.  UxROM
3.  CNROM

Planned mapper expansion is tracked in the umbrella repository's
[mapper specs](https://github.com/Kautenja/gym-nes/tree/main/specs/mappers).

# Disclaimer

**This project is provided for educational purposes only. It is not
affiliated with and has not been approved by Nintendo.**
