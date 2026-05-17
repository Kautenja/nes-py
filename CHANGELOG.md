# Changelog

Notable release notes for `nes-py` are collected here from the
[GitHub Releases](https://github.com/Kautenja/nes-py/releases) history.

## [9.0.0](https://github.com/Kautenja/nes-py/releases/tag/9.0.0) - 2026-05-17

**Migrate to Gymnasium**

- Replaced the legacy OpenAI Gym runtime dependency with Gymnasium.
- Updated `NESEnv.reset` to return `(observation, info)`.
- Updated `NESEnv.step` to return
  `(observation, reward, terminated, truncated, info)`.
- Moved render mode selection to environment construction.

## [8.2.1](https://github.com/Kautenja/nes-py/releases/tag/8.2.1) - 2022-06-21

**Resolve warnings from gym >0.20**

- Implemented `seed`, `options`, and `return_info` keyword arguments for
  `NESEnv.reset`.
- Only `seed` is supported; `options` and `return_info` have no effect.

## [8.2.0](https://github.com/Kautenja/nes-py/releases/tag/8.2.0) - 2022-06-21

**Support for gym 0.20**

- Updated support for changes in gym 0.20.
- Explicitly cast reward signals to `float`.
- Explicitly cast done signals to `bool`.

## [8.1.9](https://github.com/Kautenja/nes-py/releases/tag/8.1.9) - 2022-06-07

**Support for M1 Mac**

- Updated builds to support M1 Macs.

## [8.1.8](https://github.com/Kautenja/nes-py/releases/tag/8.1.8) - 2021-09-05

**Fix for PyPy interpreter**

- Added a simple type cast fix to support PyPy's NumPy implementation.
- Officially supported Python 3.9.
- Deprecated official support for Ubuntu 14.04.
- Removed macOS from Travis CI builds because the payment model changed for
  open source projects and macOS/Windows builds became too expensive to run on
  Travis CI.

## [8.1.7](https://github.com/Kautenja/nes-py/releases/tag/8.1.7) - 2021-07-28

**Fix pyglet for macOS Big Sur**

- Updated `pyglet` to 1.5.11.

## [8.1.6](https://github.com/Kautenja/nes-py/releases/tag/8.1.6) - 2020-08-26

**Fix NumPy version**

- Fixed the NumPy version for Linux builds.

## [8.1.5](https://github.com/Kautenja/nes-py/releases/tag/8.1.5) - 2020-08-26

**Fix version mismatch for pyglet with gym**

- Resolved a version mismatch for the `pyglet` dependency between `nes-py` and
  `gym`.
- Dropped the `pyglet==1.5.5` requirement and mirrored the version range used by
  `gym==0.17.2`.

## [8.1.4](https://github.com/Kautenja/nes-py/releases/tag/8.1.4) - 2020-06-16

**Update README**

- Removed outdated documentation for unsupported features.
- Added screenshots.

## [8.1.3](https://github.com/Kautenja/nes-py/releases/tag/8.1.3) - 2020-06-16

**Officially remove support for rendering in Python threads**

- Moved pyglet import handling into `ImageViewer`.
- Added logic to detect `ImageViewer` creation from Python threads other than
  the main thread and raise `RuntimeError`.
- Documented render logic for multiprocessing environments.

## [8.1.2](https://github.com/Kautenja/nes-py/releases/tag/8.1.2) - 2020-06-12

**Fix pyglet frame limiting**

- Fixed an issue caused by pyglet deprecating its frame limiting logic.

## [8.1.0](https://github.com/Kautenja/nes-py/releases/tag/8.1.0) - 2019-07-16

**Switch from clang++ to g++**

- Switched Linux builds from `clang++` to `g++`, producing a reported 25%
  speedup on Linux machines.
- Noted no speedup on macOS.
- Noted that neither `g++` nor `clang++` was available in Windows through MSVC.

## [8.0.2](https://github.com/Kautenja/nes-py/releases/tag/8.0.2) - 2019-06-03

**Fix render glitch**

- Changed `SCANLINE_END_CYCLE` from 340 to 341 to resolve a scanline glitch.

## [8.0.0](https://github.com/Kautenja/nes-py/releases/tag/8.0.0) - 2019-06-02

**Rename BinarySpaceToDiscreteSpaceEnv to JoypadSpace**

- Renamed `BinarySpaceToDiscreteSpaceEnv` to `JoypadSpace`.

## [7.0.0](https://github.com/Kautenja/nes-py/releases/tag/7.0.0) - 2019-05-22

**Deprecate Python 2**

- Deprecated Python 2 support.
- Deprecated pygame module usage in favor of pyglet.

## [6.2.2](https://github.com/Kautenja/nes-py/releases/tag/6.2.2) - 2019-05-22

**Fix spaces**

- Fixed spaces to work with newer versions of gym.

## [6.2.1](https://github.com/Kautenja/nes-py/releases/tag/6.2.1) - 2019-01-21

**Rename RNG to work with Gym**

- Renamed RNG-related code for Gym compatibility.

## [6.2.0](https://github.com/Kautenja/nes-py/releases/tag/6.2.0) - 2019-01-21

**RNG**

- Added a NumPy-based RNG to the environment.

## [6.1.0](https://github.com/Kautenja/nes-py/releases/tag/6.1.0) - 2019-01-21

**get_action_meanings method implemented**

- Implemented `get_action_meanings` for compatibility with gym code such as the
  noop reset wrapper.

## [6.0.0](https://github.com/Kautenja/nes-py/releases/tag/6.0.0) - 2019-01-21

**Remove wrappers**

- Removed wrappers that were outside the scope of the project.

## [5.1.0](https://github.com/Kautenja/nes-py/releases/tag/5.1.0) - 2019-01-18

**2 controller support**

- Added preliminary support for two controllers.
- Improved access speed for controller state buffers from Python.

## [5.0.2](https://github.com/Kautenja/nes-py/releases/tag/5.0.2) - 2019-01-09

**Cleanup C++ backend**

- Reviewed and cleaned up the C++ backend.

## [5.0.1](https://github.com/Kautenja/nes-py/releases/tag/5.0.1) - 2019-01-06

**Make _skip public in FrameSkipEnv**

- Made `_skip` public as `skip` so end users can mutate it more intuitively.

## [5.0.0](https://github.com/Kautenja/nes-py/releases/tag/5.0.0) - 2019-01-06

**Remove unnecessary features**

- Moved frame skip to a separate environment wrapper.
- Removed the frame limiting mechanism.

## [4.1.2](https://github.com/Kautenja/nes-py/releases/tag/4.1.2) - 2019-01-06

**Fix restore feature**

- Set a flag when a backup is created so `reset` can use `_backup` instead of
  `_LIB.reset(self._env)`.

## [4.1.1](https://github.com/Kautenja/nes-py/releases/tag/4.1.1) - 2019-01-06

**Backup and restore feature**

- Implemented state backup and restore for SimpleNES.

## [4.0.1](https://github.com/Kautenja/nes-py/releases/tag/4.0.1) - 2019-01-05

**Fix dependencies for Python 2**

- Fixed the matplotlib dependency by using `>=2.0.2` instead of `>=2.3.2`.

## [4.0.0](https://github.com/Kautenja/nes-py/releases/tag/4.0.0) - 2019-01-05

**Better RAM accessor**

- Used a direct buffer to create a NumPy vector around emulator RAM instead of
  issuing reads and writes through the ctypes API.

## [3.0.3](https://github.com/Kautenja/nes-py/releases/tag/3.0.3) - 2019-01-05

**Resolve issue in setup.py for Windows**

- Resolved an issue in `setup.py` that caused Windows installs to fail.

## [3.0.2](https://github.com/Kautenja/nes-py/releases/tag/3.0.2) - 2019-01-05

**Resolve compiler warning**

- Commented out an unused variable to resolve a `clang++` compiler warning.

## [3.0.1](https://github.com/Kautenja/nes-py/releases/tag/3.0.1) - 2019-01-05

**Code optimization**

- Optimized and refactored code for a reported 20% speedup.

## [3.0.0](https://github.com/Kautenja/nes-py/releases/tag/3.0.0) - 2019-01-05

**SimpleNES backend**

- Replaced LaiNES with SimpleNES.

## [1.1.0](https://github.com/Kautenja/nes-py/releases/tag/1.1.0) - 2018-09-10

**Allow resizable windows**

- Implemented resizable windows in the human play script.
- Implemented resizable windows in `_simple_image_viewer`, used by `render`.

## [1.0.0](https://github.com/Kautenja/nes-py/releases/tag/1.0.0) - 2018-09-10

**Pass a done parameter to `_did_step`**

- Passed a `done` parameter to `_did_step` so callbacks can base logic on an
  episode's completion state.

## [0.11.1](https://github.com/Kautenja/nes-py/releases/tag/0.11.1) - 2018-09-10

**Resolve bug with backup/restore state feature**

- Unwrapped the environment when calling backup and restore.

## [0.11.0](https://github.com/Kautenja/nes-py/releases/tag/0.11.0) - 2018-09-10

**Add Windows support**

- Officially supported Windows.

## [0.10.3](https://github.com/Kautenja/nes-py/releases/tag/0.10.3) - 2018-08-13

**Resolve memory leak**

- Fixed a memory leak where backup and restore created dangling pointers without
  deleting them first.

## [0.10.2](https://github.com/Kautenja/nes-py/releases/tag/0.10.2) - 2018-08-12

**Virtual mapper destructor**

- Used a virtual destructor to resolve a compiler error.

## [0.10.1](https://github.com/Kautenja/nes-py/releases/tag/0.10.1) - 2018-08-12

**Fix joypad bug on Debian**

- Fixed a bug where the joypad was not initialized correctly on Debian systems.

## [0.10.0](https://github.com/Kautenja/nes-py/releases/tag/0.10.0) - 2018-08-12

**Backup and restore feature**

- Implemented `_backup` to create a backup state that is restored automatically
  on subsequent calls to `reset`.
- Implemented `_del_backup` to delete the backup state.
- Implemented `_restore` to restore the backed up state into the machine.

## [0.9.0](https://github.com/Kautenja/nes-py/releases/tag/0.9.0) - 2018-07-22

**Reward plot in play_human**

- Improved `play_human` to simplify client environments.

## [0.8.9](https://github.com/Kautenja/nes-py/releases/tag/0.8.9) - 2018-07-22

**Bug fixes**

- Fixed `get_keys_to_actions` after using `BinarySpaceToDiscreteSpaceEnv`.
- Fixed `_did_reset`, which had accidentally been named `_did_reset_`.
- Removed unnecessary binary space.
- Removed the num buttons function from C++ code.

## [0.8.8](https://github.com/Kautenja/nes-py/releases/tag/0.8.8) - 2018-07-22

**Fix Python 2 support**

- Removed type hints and resolved minor issues to support Python 2.

## [0.8.4](https://github.com/Kautenja/nes-py/releases/tag/0.8.4) - 2018-07-21

**Beta**

- Released beta.

## [0.8.3](https://github.com/Kautenja/nes-py/releases/tag/0.8.3) - 2018-07-21

**Build script features**

- Added Travis CI features to simplify deployment.

## [0.8.2](https://github.com/Kautenja/nes-py/releases/tag/0.8.2) - 2018-07-21

**Fix issue with Python 2.7**

- Resolved the iNES magic value check by using `bytearray` instead of `bytes`.

## [0.8.0](https://github.com/Kautenja/nes-py/releases/tag/0.8.0) - 2018-07-21

**Wrappers**

- Implemented reward cache.
- Implemented binary action space to discrete action space.
- Implemented downsample.
- Implemented clip reward.
- Implemented normalize reward.
- Implemented frame stack.
- Implemented death penalty.

## [0.7.0](https://github.com/Kautenja/nes-py/releases/tag/0.7.0) - 2018-07-20

**Reward bound range**

- Bounded rewards into the defined range.

## [0.6.0](https://github.com/Kautenja/nes-py/releases/tag/0.6.0) - 2018-07-20

**Max steps feature**

- Defined a mechanism to limit the number of steps in an episode.

## [0.5.3](https://github.com/Kautenja/nes-py/releases/tag/0.5.3) - 2018-07-20

**Alpha release**

- Released full alpha.

## [0.5.1](https://github.com/Kautenja/nes-py/releases/tag/0.5.1) - 2018-07-20

**Frameskip mechanism**

- Implemented a frameskip mechanism.

## [0.4.1](https://github.com/Kautenja/nes-py/releases/tag/0.4.1) - 2018-07-20

**Info callback**

- Added a callback for info.

## [0.4.0](https://github.com/Kautenja/nes-py/releases/tag/0.4.0) - 2018-07-20

**Implement lifecycle callbacks**

- Implemented `_frame_advance`.
- Implemented `_will_reset`.
- Implemented `_did_reset`.
- Implemented `_did_step`.

## [0.3.0](https://github.com/Kautenja/nes-py/releases/tag/0.3.0) - 2018-07-20

**Memory access**

- Defined an API for accessing NES memory in Python.

## [0.2.8](https://github.com/Kautenja/nes-py/releases/tag/0.2.8) - 2018-07-20

**Fix README**

- Corrected a broken README link when publishing to PyPI.

## [0.2.7](https://github.com/Kautenja/nes-py/releases/tag/0.2.7) - 2018-07-20

**Fix CLI**

- Fixed the CLI to require the ROM path.

## [0.2.6](https://github.com/Kautenja/nes-py/releases/tag/0.2.6) - 2018-07-19

**Remove very-good-setup-tools-git-version**

- Removed `very-good-setup-tools-git-version` because Debian systems installing
  from source tried to use a git version that is not available in source
  distributions.

## [0.2.4](https://github.com/Kautenja/nes-py/releases/tag/0.2.4) - 2018-07-19

**Create deployment script for Darwin and Debian systems**

- Created `setup.py` to compile the LaiNES dependency into a shared object.
- Tested on Darwin and Debian systems.

## [0.1.1](https://github.com/Kautenja/nes-py/releases/tag/0.1.1) - 2018-07-18

**Resolve licensing issues**

- Fixed licensing in `LICENSE` and `setup.py`.

## [0.1.0](https://github.com/Kautenja/nes-py/releases/tag/0.1.0) - 2018-07-18

**Python interface to play games**

- Defined a complete Python interface, including a CLI, to load and play games
  on the NES.

## [0.0.0](https://github.com/Kautenja/nes-py/releases/tag/0.0.0) - 2018-07-17

**Initial release**

- Released simple C++ code based on the
  [LaiNES implementation](https://github.com/AndreaOrru/LaiNES).
