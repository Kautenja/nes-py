<center>
<img 
    src="https://user-images.githubusercontent.com/2184469/42917952-58525e1a-8ad1-11e8-81ad-51333da439fa.png"
    align="center"
    width="50%"
/>
</center>

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

## Usage

### Requirements

nes-py should run on any Unix system that is compatible with the following
tools.

-   SConstruct
-   C++11 compatible compiler (e.g. `clang++`)

#### Debian-based systems:

```shell
sudo apt-get install clang scons
```

#### MacOS:

```shell
brew install scons
```

### Compilation

```shell
scons
```

## Compatibility

LaiNES implements the most common mappers, which should be enough for a good
percentage of the games:

-   NROM (Mapper 000)
-   MMC1 / SxROM (Mapper 001)
-   UxROM (Mapper 002)
-   CNROM (Mapper 003)
-   MMC3, MMC6 / TxROM (Mapper 004)

You can check the compatibility for each ROM in the following
[list](http://tuxnes.sourceforge.net/nesmapper.txt)
