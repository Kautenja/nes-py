# Contributing to nes-py

Help improve `nes-py` through bug reports, documentation, tests, and code.
This guide takes you from a fresh checkout to a tested contribution and
explains the architecture and compatibility requirements used in review.

- [Set up your environment](#set-up-your-environment)
- [Understand the architecture](#architecture)
- [Build and test](#development-and-testing)
- [Choose validation for your change](#choosing-validation)
- [Prepare a release](#prepare-a-release)
- [Submit a pull request](#submit-a-pull-request)

## Before You Start

Search the [existing issues](https://github.com/Kautenja/nes-py/issues) before
reporting a bug or proposing a feature. Include the operating system, CPU
architecture, Python and `nes-py` versions, install method, and compiler
version when relevant. Provide a minimal reproducer, expected versus observed
behavior, and the complete relevant error output. For ROM-dependent problems,
identify the game or homebrew revision, mapper, and actions needed to reproduce
the issue. Do not upload commercial ROMs with reports or contributions.
Discuss substantial API or emulator behavior changes in an issue first.

Read the [README](README.md) and the relevant design and benchmark notes in
[docs](docs). Preserve the attribution and license notices in the source and
[LICENSE](LICENSE), and read [LICENSING.md](LICENSING.md) for third-party
provenance and scope. Follow the Python style guidance linked by the
[pull request template](.github/PULL_REQUEST_TEMPLATE.md), and match the
surrounding Cython and C++ conventions. Keep formatting changes focused on the
code being changed.

## Set Up Your Environment

| Work | Required tools |
| --- | --- |
| Python package and extension | Git, CPython 3.13 or 3.14, a C++14 compiler, and Python development headers |
| Native tests and benchmarks | CMake 3.21 or newer, a C++14 compiler, and Git/network access for the pinned Catch2 dependency |
| Interactive rendering | Package dependencies and a working desktop/OpenGL session |
| Markdown documentation | A text editor and Git; no native build required |

The [CI workflow](.github/workflows/ci.yml) records the supported toolchains:
GCC on Linux, Xcode's Clang on macOS, and Visual Studio 2022 C++ tools on
Windows. Linux distributions may package Python development headers and
`venv` separately. Use a compiler and interpreter with matching architectures.

### Get the Source

Fork the repository on GitHub, then substitute your fork's clone URL below:

```shell
git clone https://github.com/Kautenja/nes-py.git
cd nes-py
git switch -c docs/contributor-setup
```

Choose a branch name describing your change. Run the remaining commands from
the repository root unless stated otherwise.

### Install for Development

Create a virtual environment using a supported interpreter. On macOS or Linux:

```shell
python3.13 -m venv .venv
source .venv/bin/activate
```

On Windows, use PowerShell:

```powershell
py -3.13 -m venv .venv
.venv\Scripts\Activate.ps1
```

Python 3.14 can be used instead. With the environment active:

```shell
python -m pip install --upgrade pip
python -m pip install -r requirements.txt
python -m pip install --editable . --config-settings=editable.mode=inplace
python -c "import nes_py; import nes_py._native"
```

The editable install builds the native extension in place so tests run against
the checkout. Repeat the editable-install command after changing Cython or
C++ sources; editing those files alone does not rebuild the extension.
Runtime dependencies and isolated build requirements are declared in
[pyproject.toml](pyproject.toml); [requirements.txt](requirements.txt) contains
development tools. The build backend manages CMake for Python package builds.

On macOS or Linux, [main.sh](main.sh) provides shortcuts:

```shell
PYTHON=python3.13 ./main.sh install
./main.sh test
```

The script prefers an existing `.venv/bin/python`; `PYTHON` selects the
interpreter when creating a new environment. Use the direct Python commands
above for Windows.

## Architecture

### Source Map

- [nes_py/nes_env.py](nes_py/nes_env.py): the scalar Gymnasium environment,
  reset/step hooks, observations, RAM access, snapshots, and rendering.
- [nes_py/vector_env.py](nes_py/vector_env.py): same-ROM vector emulation and
  batch observation/RAM helpers. Game-specific reward, termination, and info
  logic stays outside the native vector emulator.
- [nes_py/_native.pyx](nes_py/_native.pyx): Cython bindings, native ownership,
  NumPy views and copies, and scalar/vector execution boundaries.
- [nes_emu/include/nes_emu](nes_emu/include/nes_emu) and
  [nes_emu/src/nes_emu](nes_emu/src/nes_emu): the C++ emulator, CPU, PPU,
  buses, cartridge loading, controllers, and mappers.
- [nes_py/_rom.py](nes_py/_rom.py) and [nes_py/ram.py](nes_py/ram.py): ROM
  metadata parsing and RAM read specifications.
- [nes_py/wrappers](nes_py/wrappers), [nes_py/play.py](nes_py/play.py), and
  [nes_py/_image_viewer.py](nes_py/_image_viewer.py): action wrappers,
  command-line play, and windowed rendering.
- [nes_py/tests](nes_py/tests), [nes_emu/test](nes_emu/test), and
  [nes_emu/benchmark](nes_emu/benchmark): Python application tests, native
  Catch2 regressions, and native benchmarks. [nes_py/speedtest.py](nes_py/speedtest.py)
  provides end-to-end benchmark commands.
- [CMakeLists.txt](CMakeLists.txt) and [pyproject.toml](pyproject.toml): native
  targets, Cython generation, package metadata, and distribution configuration.

### Compatibility and Ownership

Preserve Gymnasium's `reset()` result `(observation, info)` and `step()` result
`(observation, reward, terminated, truncated, info)`, including seeding,
render-mode behavior, and subclass hooks used by downstream environments.
Coordinate intentional API changes with documentation and migration guidance.

The default RGB observation is a `(240, 256, 3)` `uint8` view over native
storage. It is strided and can change as the emulator advances. Preserve its
ownership and lifetime guarantees; explicit contiguous RGB and grayscale
helpers provide copies. Check output shape, dtype, writability, and buffer
lifetime when modifying bindings or introducing GIL-free work.

Snapshots are opaque same-process checkpoints, not a portable save-state
format. Changes to CPU, PPU, mapper, or bus state need restore and deterministic
replay checks. Preserve mapper callbacks, bank mappings, and cache invalidation
across reset and restore. The [snapshot notes](docs/explicit-state-snapshot-api.md)
and [vector notes](docs/vector-native-emulator.md) explain these boundaries.

Keep emulator logic independent of the viewer. Windowed `human` rendering
must run on the process's main thread, and a child process must create its
own viewer. Headless imports and `rgb_array` operation should not initialize
a graphical window. Multiple emulator instances do not imply that concurrent
mutation of one instance is safe.

## Development and Testing

### Python Tests

After installing or rebuilding the editable package, run the CI test command:

```shell
python -m unittest discover .
```

For focused iteration, name a test module, for example:

```shell
python -m unittest nes_py.tests.test_rom
python -m unittest nes_py.tests.mappers.test_mapper_004_mmc3
```

Some application tests use on-disk fixtures under `nes_py/tests/games`.
Prefer deterministic synthetic fixtures for new regressions; helpers live in
[mapper_fixtures.py](nes_py/tests/mapper_fixtures.py) and
[synthetic_rom.hpp](nes_emu/test/nes_emu/support/synthetic_rom.hpp). Identify
any additional required fixtures and report missing or skipped coverage.
Reproduce a bug before fixing it when practical, and test observable behavior
rather than mirroring implementation details.

### Native Tests

Native tests are opt-in and do not require the Python extension. The first
configuration fetches Catch2 3.5.4. Normal package builds leave these targets
disabled and do not fetch Catch2.

```shell
cmake -S . -B build/nes-emu-debug -DCMAKE_BUILD_TYPE=Debug -DNES_EMU_BUILD_TESTS=ON
cmake --build build/nes-emu-debug --config Debug --target nes_emu_tests
./build/nes-emu-debug/nes_emu_tests
```

Some frame-comparison tests resolve ROM paths relative to the working
directory and require fixtures under `nes_py/tests/games`. Run the executable
from the repository root as shown above. The current CTest registration uses
the build directory as its working directory, so those fixture checks fail
when invoked through `ctest --test-dir build/nes-emu-debug`.
With a multi-configuration generator such as Visual Studio, use
`build/nes-emu-debug/Debug/nes_emu_tests.exe`. Check the output for missing
fixtures; passing synthetic tests does not establish gameplay correctness.

### Gameplay and Rendering

Use a legally obtained ROM to exercise affected behavior. Replace the
placeholder with its local path:

```shell
python -m nes_py.play --rom <path_to_rom> --mode random --steps 500 --no-render
python -m nes_py.play --rom <path_to_rom>
```

For timing or mapper changes, record the ROM revision, mapper, input sequence,
and observed frame or RAM behavior. For rendering changes, check window
creation, keyboard input, window closure, and headless operation. A headless
test does not verify window behavior.

### Benchmarks

Build and run native benchmarks in Release mode, from the repository root:

```shell
cmake -S . -B build/nes-emu-release -DCMAKE_BUILD_TYPE=Release -DNES_EMU_BUILD_BENCHMARKS=ON
cmake --build build/nes-emu-release --config Release --target nes_emu_benchmarks
./build/nes-emu-release/nes_emu_benchmarks
```

For Visual Studio, use `build/nes-emu-release/Release/nes_emu_benchmarks.exe`.
Some profiles require the on-disk game fixtures. End-to-end Python examples:

```shell
python -m nes_py.speedtest --rom <path_to_rom> --steps 5000 --seed 123 --json --no-progress
python -m nes_py.speedtest --rom <path_to_rom> --vector-profile --runs 5 --env-counts 1,2,4,8,16 --instrumentation --json --no-progress
```

Use matched before/after workloads, compiler settings, warm-up, seeds, and
repeated runs. Record the platform, interpreter, fixture, and variation with
results. Benchmark throughput is informational; a single timing or passing
build does not establish a speedup. Keep design decisions and reproducible
measurements in [docs](docs).

### Distribution Builds and CI

Build both a source distribution and a wheel with:

```shell
python -m build
```

Outputs go to `dist/`. On macOS or Linux, `./main.sh deployment` cleans local
build products before building both distributions. Its cleanup also removes
the in-place native extension and native build directories; rebuild the
editable install before returning to Python tests.

The [CI workflow](.github/workflows/ci.yml) runs Python tests and distribution
builds for CPython 3.13 and 3.14 on Linux x64/arm64, macOS x64/arm64, and
Windows x64. It runs on pull requests, pushes to `master`, and tag pushes.
The opt-in native Catch2 tests and benchmarks are not run by this workflow;
include local native results for changes that need them.

### Choosing Validation

| Change | Relevant validation |
| --- | --- |
| Markdown guidance | Check links, paths, commands, and the complete diff; run `git diff --check` |
| Python API, wrappers, or CLI | Run focused tests and the Python suite; check affected public examples |
| Cython or buffer ownership | Rebuild the extension; check scalar/vector operations, invalid inputs, views, copies, and close behavior |
| CPU, PPU, or mapper behavior | Run native and Python regressions; check affected gameplay, timing, reset, and snapshot replay |
| Rendering or process behavior | Check interactive rendering and headless operation, including affected thread/process paths |
| Performance | Establish correctness first, then compare repeated matched native and end-to-end workloads |
| Packaging or build configuration | Build sdist/wheel; install the wheel and build from the extracted sdist in clean environments outside the checkout |

## Prepare a Release

Update the version in [pyproject.toml](pyproject.toml), document user-visible
changes, and validate the intended release revision. Inspect the distributions
and verify clean installation and native imports. Preserve the established
2018 citation in [CITATION.bib](CITATION.bib), [CITATION.cff](CITATION.cff), and
the README; routine software releases do not create a new bibliographic work.

Create a new tag matching the package version, optionally prefixed with `v`.
Do not move or reuse a published tag. Tag CI builds distributions and creates
or updates a GitHub release with its artifacts.

PyPI publishing is handled separately by
[Publish to PyPI](.github/workflows/publish.yml). It runs when a GitHub release
is published or when manually dispatched against a matching tag, builds the
sdist and supported wheels, and publishes through the `pypi` environment's
trusted publisher. Under GitHub's
[workflow trigger rules](https://docs.github.com/en/actions/how-tos/write-workflows/choose-when-workflows-run/trigger-a-workflow#triggering-a-workflow-from-a-workflow),
releases created by another workflow's default GitHub token do not
automatically trigger a new release workflow; use an explicit dispatch
against the release tag if publishing did not start. Confirm both
the workflow result and the package's availability on PyPI before announcing
the release. `./main.sh ship` prepares local distributions; it does not upload
them to PyPI.

## Submit a Pull Request

1. Keep one coherent change per contribution. Update affected documentation
   alongside behavior changes, retain upstream notices, and avoid unrelated
   formatting or dependency upgrades.
2. Review the intended changes:

   ```shell
   git diff --check
   git diff
   git status --short
   ```

3. Commit only the intended files, push your branch to your fork, and open a
   pull request against `master`. Use the
   [pull request template](.github/PULL_REQUEST_TEMPLATE.md) to describe the
   problem, resulting behavior, related issue, and validation commands/results.
4. Include OS, CPU architecture, Python/compiler versions, and fixture details
   where relevant. Add screenshots for visible changes. Distinguish builds,
   automated checks, gameplay checks, and measurements; state skipped checks
   and unresolved failures explicitly.
