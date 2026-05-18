# Native Batch RAM Info Reads

Spec 051 adds a generic RAM read descriptor helper for scalar `NESEnv` and
same-ROM `VectorNESEmulator` workloads. Game wrappers still own address lists,
reward logic, termination logic, and info dictionaries.

## API Proposal

`env.ram_values(specs, output=None)` returns a C-contiguous uint32 array.
`VectorNESEmulator.ram_values(specs, output=None)` returns a
`(num_envs, len(specs))` uint32 array.

Descriptors are intentionally small:

- `address`: read one byte from CPU RAM.
- `(address, size)`: read a little-endian unsigned integer.
- `(address, size, encoding)`: read using `byte`, `little`, `big`, `bcd`, or
  `digits`.
- `{"address": ..., "size": ..., "encoding": ...}`: mapping form.

The helper validates address ranges, read sizes, encoding names, dtype, output
shape, contiguity, writability, reset/restore usability, and close behavior.
The steady-state native path writes directly into caller-provided output when
one is supplied.

## Profiling

Host: macOS-26.3 arm64, CPython 3.14.2, clang, editable release-extension
build. ROM: `nes_py/tests/games/super-mario-bros-1.nes`.

Command shape:

```sh
.venv/bin/python - <<'PY'
from nes_py.speedtest import run_ram_profile
for _ in range(5):
    run_ram_profile(
        'nes_py/tests/games/super-mario-bros-1.nes',
        steps=100,
        warmup_steps=20,
        seed=111,
        action_policy='noop',
    )
PY
```

Median throughput:

| Operation | Median steps/s | Min | Max |
| --- | ---: | ---: | ---: |
| python_indexing | 354190.37 | 344530.58 | 355923.81 |
| numpy_take | 1128871.68 | 1103448.02 | 1170740.61 |
| native_batch | 169599.31 | 166067.17 | 170624.01 |
| step_python_indexing | 1499.89 | 1405.36 | 1506.29 |
| step_native_batch | 1484.41 | 1479.67 | 1499.47 |

The native helper is not an isolated RAM-read speedup on this machine. The
Python-to-native call and descriptor generality are more expensive than plain
NumPy gather for tiny address sets. In full `step + info` style loops the
result was within roughly 1% of Python indexing.

## Decision

The implementation is kept as a shared primitive, not as a proven speedup. It
centralizes validation, supports reusable outputs, gives vector workloads one
generic RAM readback path, and showed no meaningful regression in frame-heavy
representative loops. Wrappers should still profile their own address sets
before replacing direct `env.ram[...]` indexing.
