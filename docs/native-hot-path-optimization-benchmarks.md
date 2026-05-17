# Native Hot Path Optimization Benchmarks

Spec 016 compared the pre-change `ralph-dev` tip (`108e224`) with the
working tree that replaces main-bus hash callback dispatch with direct device
dispatch and fixed-size RAM storage.

Platform:

- macOS 26.3 arm64
- CPython 3.14.2
- clang via Python sysconfig
- Editable build in `.venv`

## Profiling Evidence

The steady-state Python profile was captured after imports with:

```sh
.venv/bin/python - <<'PY'
import cProfile
import io
import pstats
from nes_py.speedtest import BenchmarkConfig, run_benchmark

config = BenchmarkConfig(
    rom='nes_py/tests/games/super-mario-bros-1.nes',
    steps=300,
    warmup_steps=0,
    action_policy='noop',
    progress=False,
)
profile = cProfile.Profile()
profile.enable()
result = run_benchmark(config)
profile.disable()
stream = io.StringIO()
pstats.Stats(profile, stream=stream).strip_dirs().sort_stats(
    'cumulative'
).print_stats(15)
print(result.to_dict())
print(stream.getvalue())
PY
```

Before the change, 300 measured frame steps spent 0.326s of 0.327s under
`nes_env.py:253(step)`, which is the native `NativeEmulator.frame_advance`
boundary. The comparable after profile spent 0.327s of 0.328s under the same
boundary. That kept the optimization target inside native frame-step work
rather than Python loop or benchmark overhead.

Source-level profiling of the native frame-step path showed `MainBus::read`
and `MainBus::write` dispatching CPU-visible PPU/controller/OAMDMA register
accesses through `unordered_map::count`, `unordered_map::at`, and
`std::function` callbacks. The landed optimization wires the fixed emulator
devices directly into `MainBus`, keeps callback arrays only as standalone
fallbacks, and stores CPU RAM in `std::array` because it is always 2 KiB.

## Benchmark Commands

The comparison used three runs and median throughput for each profile. The
baseline was built from a detached worktree at `108e224`, then the edited tree
was rebuilt in the same virtualenv.

High-level Cython/native binding profiles used:

```py
run_benchmark(BenchmarkConfig(
    rom='nes_py/tests/games/super-mario-bros-1.nes',
    steps=1000,
    warmup_steps=100,
    seed=7,
    action_policy='noop',
    progress=False,
))
run_benchmark(BenchmarkConfig(
    rom='nes_py/tests/games/super-mario-bros-1.nes',
    steps=1000,
    warmup_steps=100,
    seed=7,
    action_policy='noop',
    render_mode='rgb_array',
    progress=False,
))
run_benchmark(BenchmarkConfig(
    rom='nes_py/tests/games/the-legend-of-zelda.nes',
    steps=1000,
    warmup_steps=100,
    seed=7,
    action_policy='noop',
    backup_interval=50,
    restore_interval=70,
    progress=False,
))
```

Mapper profiles used synthetic mapper 0-3 fixtures with
`run_mapper_profile(..., steps=100, warmup_steps=20)`.

Mapper timing/IRQ hook profiling used the native benchmark target. In the
current tree this is built separately from the Python extension with
`-DNES_EMU_BUILD_BENCHMARKS=ON` and run through the `nes_emu_benchmarks`
executable, so native microbenchmarks do not expand the Python package API.

```sh
cmake --build build/nes-emu-release --config Release --target nes_emu_benchmarks
build/nes-emu-release/nes_emu_benchmarks --benchmark-samples 1 --benchmark-resamples 1
```

Umbrella specs 017 and 018 were still incomplete, so there were no landed
CPU/bus/frame-timing or PPU address/render benchmark suites to include beyond
the current step and render/view profiles above.

## Results

| Profile | Before median | After median | Change |
| --- | ---: | ---: | ---: |
| SMB1 step-heavy | 855.12 steps/s | 856.72 steps/s | +0.19% |
| SMB1 render/view-heavy | 857.20 frames/s | 856.32 frames/s | -0.10% |
| Zelda backup/restore-heavy | 805.34 steps/s | 804.96 steps/s | -0.05% |
| Mapper hook smoke profile | 677,097.77 iter/s | 738,052.70 iter/s | +9.00% |

Current mapper profile medians:

| Mapper | Operation | Before | After | Change |
| ---: | --- | ---: | ---: | ---: |
| 0 | reset | 5,298,013.75 | 5,321,415.04 | +0.44% |
| 0 | step | 839.87 | 839.82 | -0.00% |
| 0 | render_rgb_array | 12,244,397.46 | 12,371,633.73 | +1.04% |
| 0 | backup_restore | 83,612.04 | 88,144.56 | +5.42% |
| 1 | reset | 5,251,550.31 | 5,298,013.75 | +0.88% |
| 1 | step | 873.07 | 878.22 | +0.59% |
| 1 | render_rgb_array | 12,061,296.27 | 12,307,642.68 | +2.04% |
| 1 | backup_restore | 83,356.53 | 87,732.15 | +5.25% |
| 2 | reset | 5,298,013.75 | 4,958,587.45 | -6.41% |
| 2 | step | 865.96 | 879.28 | +1.54% |
| 2 | render_rgb_array | 12,059,856.93 | 12,307,642.68 | +2.05% |
| 2 | backup_restore | 84,856.65 | 86,104.83 | +1.47% |
| 3 | reset | 5,298,013.75 | 5,298,013.75 | +0.00% |
| 3 | step | 867.67 | 867.58 | -0.01% |
| 3 | render_rgb_array | 12,307,730.85 | 12,307,730.85 | +0.00% |
| 3 | backup_restore | 90,675.56 | 89,448.80 | -1.35% |

The reset and render microbenchmarks complete in very small absolute times, so
their larger percentage swings are noisy. The step-heavy, render-heavy, and
backup/restore-heavy full-ROM profiles did not show a meaningful regression,
and all behavior checks passed after the optimization.
