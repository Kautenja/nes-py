# Explicit State Snapshot API

Spec 053 adds an opaque public snapshot API on top of the current
mapper-aware native backup model.

## API

Scalar:

```python
snapshot = env.dump_state()
env.load_state(snapshot)
```

Vector:

```python
snapshot = vector.dump_state(index)
vector.load_state(index, snapshot)
```

Snapshots are Python-visible opaque objects backed by `NES::Emulator::Snapshot`.
They are not NumPy views, raw pointers, or a stable cross-version save-state
format. They are intended for same-process branching, reset-to-checkpoint,
debugging, and vector reset plumbing.

## Lifetime and Compatibility

- A snapshot owns copies of CPU state, main-bus RAM, picture-bus RAM/palette,
  PPU state including the screen buffer and render caches, and a cloned mapper.
- Mapper callbacks are not copied. `load_state` clones the snapshot mapper into
  the target emulator and rewires IRQ callbacks plus main/picture bus mapper
  pointers.
- Direct PRG and CHR read pages are refreshed by mapper rewiring. Picture-bus
  mirroring is synchronized after load.
- Existing `_backup()` and `_restore()` behavior is preserved and now uses the
  same state composition internally.
- Loading an invalid Python object raises `TypeError`; loading after close
  raises `ValueError`.

## Coverage

Python tests round-trip snapshots across the supported public mapper fixtures:
0, 1, 2, 3, 4, 5, 7, 9, and 69. The tests verify screen state, RAM state,
controller-sensitive continuation behavior, mapper bank continuation, mirroring
state through rendered continuation, and instruction-batched continuation.
Existing backup/restore tests continue to exercise the private slot API.

## Benchmark Summary

Host: macOS-26.3 arm64, CPython 3.14.2, clang, editable release-extension
build. ROM: `super-mario-bros-1.nes`. Command shape:

```sh
.venv/bin/python - <<'PY'
from nes_py.nes_env import NESEnv
# 5 runs, 200 measured iterations, 20 warmups:
# measure dump_state, load_state, _backup/_restore, dump+load round trips.
PY
```

Median latency:

| Operation | Median us | Min | Max |
| --- | ---: | ---: | ---: |
| dump_state | 23.29 | 23.05 | 23.57 |
| load_state | 10.05 | 9.30 | 10.85 |
| _backup + _restore | 27.74 | 26.99 | 28.17 |
| dump_state + load_state | 34.83 | 33.65 | 35.11 |

The public snapshot round trip is slightly slower than the private slot because
it allocates an independently owned snapshot object. Normal step-only
throughput is unchanged because snapshots are opt-in and no frame path reads
snapshot state.

## Decision

The snapshot API is kept for clearer state management and vector reset
plumbing, not for raw backup/restore speed. It uses the existing safe
mapper-clone architecture, avoids raw struct copying, keeps `_backup()` and
`_restore()` intact, and provides a public checkpoint primitive without
claiming stable serialization compatibility.
