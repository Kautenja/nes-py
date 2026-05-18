# Native Vector Emulator Prototype

Spec 050 adds an opt-in `VectorNESEmulator` in `nes_py.vector_env` backed by a
Cython `NativeVectorEmulator`. The class owns multiple `NES::Emulator`
instances for one ROM and keeps game-specific reward, termination, and info
logic outside `nes-py`.

## Ownership and Lifetime

- `NativeVectorEmulator` allocates one current-branch `NES::Emulator` per
  vector slot and deletes those instances when the Python owner is deallocated.
- `close()` marks native operations closed but does not immediately invalidate
  outstanding NumPy screen or RAM views. This matches `NativeEmulator` scalar
  close behavior.
- `screens` and `rams` are tuples of per-slot zero-copy views. Batch
  observations use explicit copy helpers because independently allocated
  emulators cannot expose one contiguous zero-copy 4-D view.
- `reset()`, `reset_one(index)`, `step(actions)`, and `step_one(index, action)`
  reuse the scalar emulator reset and frame paths, including instruction
  batching, mapper hooks, PPU caches, mapper direct-read pages, backup/restore,
  and native observation helpers.
- `dump_state(index)` and `load_state(index, snapshot)` use the opaque snapshot
  API documented in `docs/explicit-state-snapshot-api.md`.

## Worker Strategy

This prototype intentionally does not create persistent worker threads. It
validates and writes the uint8 action array once, then releases the GIL while a
native loop steps each emulator serially. The choice keeps teardown simple,
avoids oversubscription, avoids idle CPU spin, and provides a stable baseline
for later threaded work. Synchronization wait time and per-worker stats are
therefore reported as zero or empty.

CPU affinity is not applied by runtime code. The benchmark CLI records
`--cpu-affinity none` or `round_robin` as an explicit experiment label so later
threaded prototypes can compare policies without changing production behavior.

## Observation Modes

- `rgb_array`: returns the tuple of existing zero-copy per-env screen views.
- `rgb_array_contiguous`: copies all slots into a reusable
  `(num_envs, 240, 256, 3)` uint8 output.
- `grayscale`: copies all slots into a reusable `(num_envs, 240, 256)` uint8
  output.
- `ram_info`: benchmark profile mode that calls the generic batch RAM helper.

## Benchmark Summary

Host: macOS-26.3 arm64, CPython 3.14.2, clang, editable release-extension
build. ROM: `nes_py/tests/games/super-mario-bros-1.nes`. Command shape:

```sh
.venv/bin/python - <<'PY'
from nes_py.speedtest import run_vector_profile
run_vector_profile(
    'nes_py/tests/games/super-mario-bros-1.nes',
    steps=20,
    warmup_steps=5,
    seed=101,
    action_policy='noop',
    env_counts=(1, 2, 4, 8, 16),
    observation_modes=('step_only', 'native_rgb_contiguous',
                       'native_grayscale', 'ram_info'),
    backends=('scalar_loop', 'native_vector'),
    runs=5,
    instrumentation=True,
)
PY
```

Representative medians in total frames/second:

| Mode | Envs | Scalar | Native vector | Change |
| --- | ---: | ---: | ---: | ---: |
| step_only | 1 | 2397.20 | 2378.76 | -0.77% |
| step_only | 4 | 2395.89 | 2394.34 | -0.06% |
| step_only | 16 | 2400.09 | 2404.63 | +0.19% |
| rgb_contiguous | 4 | 2350.73 | 2352.04 | +0.06% |
| grayscale | 4 | 2300.54 | 2309.21 | +0.38% |
| ram_info | 4 | 2333.00 | 2388.23 | +2.37% |
| ram_info | 16 | 2364.51 | 2400.39 | +1.52% |

Gymnasium baseline check, step-only profile, same host and ROM:

| Envs | Scalar | SyncVectorEnv | Native vector |
| ---: | ---: | ---: | ---: |
| 1 | 2381.06 | 1606.93 | 2377.50 |
| 4 | 2397.04 | 1625.72 | 2396.60 |
| 16 | 2400.13 | 1632.19 | 2331.12 |

`AsyncVectorEnv` works from the CLI entry point, but the compact in-process
summary script was run from stdin and Python multiprocessing could not reload
`<stdin>` as `__main__`, so those rows were omitted from the table.

## Decision

The serial native vector prototype did not meet the throughput rule for a
performance acceptance: no 10% median win appeared at 4+ envs and no 15% win
appeared at the highest count. It is kept because it provides a simpler
same-ROM native API, shared batch observation and RAM plumbing, clear timing
instrumentation, no meaningful representative regression, and a safe baseline
for future threaded experiments.
