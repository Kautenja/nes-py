# laines-py

[![build-status][]][ci-server]

[build-status]: https://travis-ci.com/Kautenja/laines-py.svg?token=FCkX2qMNHzx2qWEzZZMP&branch=master
[ci-server]: https://travis-ci.com/Kautenja/laines-py

## Usage

### Requirements

LaiNES should run on any Unix system that is compatible with the following
tools.

-   SConstruct
-   C++11 compatible compiler (e.g. `clang++`)
-   SDL2 (including `sdl2-image`)

#### Debian-based systems:

```shell
sudo apt-get install clang scons libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev
```

#### MacOS:

```shell
brew install scons sdl2 sdl2_image sdl2_ttf
```

### Compilation

<!-- TODO: update -->

```shell
git clone --recursive https://github.com/AndreaOrru/LaiNES && cd LaiNES
scons
```

### Execution

```shell
./laines <nes_rom_path>
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
