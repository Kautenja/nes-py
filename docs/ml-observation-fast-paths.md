# ML Observation Fast Paths

Spec 043 added explicit observation-copy helpers for ML training loops without
changing the default Gymnasium contract. `NESEnv.step` still returns
`env.screen`, the zero-copy `(240, 256, 3)` RGB view over the native 32-bit
screen buffer, and `render` in `rgb_array` mode still returns that same object.

The new opt-in helpers are:

- `env.observation("rgb_array")`: existing zero-copy public screen view.
- `env.observation("rgb_array_contiguous", output=None)`: C-contiguous RGB
  `uint8` copy, optionally written into a reusable output array.
- `env.observation("grayscale", output=None)`: C-contiguous 2-D `uint8` luma
  copy, optionally written into a reusable output array.

Both copy helpers read directly from the native 32-bit screen buffer in the
Cython extension. They are intended for training loops that already know they
need copied observations. They do not choose game-specific crops, resize frames,
or stack history because those choices depend on the downstream wrapper and
model. Crop/downsample modes were not kept in this pass to avoid baking a
game-specific preprocessing policy into generic `nes-py`.

## Benchmark Command

```sh
.venv/bin/python -m nes_py.speedtest --rom nes_py/tests/games/super-mario-bros-1.nes --observation-profile --steps 1000 --warmup-steps 100 --action-policy noop --json --no-progress
```

The profile measures complete `step + observation consumption` loops. Each
operation consumes one byte from the produced observation to keep all paths
accounted for.

## Local Smoke Results

Single-run profile on macOS, CPython 3.14.2, `super-mario-bros-1.nes`, noop
actions, 100 warmup steps, 1000 measured steps. Treat these as local direction,
not a portable threshold.

| Operation | Before `8e33e74` | After | Notes |
| --- | ---: | ---: | --- |
| `step` | 1106.94 steps/s | 1440.57 steps/s | Default zero-copy screen view; timing noise dominates this row. |
| `step_copy` | 1114.20 steps/s | 1117.23 steps/s | Public screen `.copy()` baseline. |
| `step_contiguous` | 1112.28 steps/s | 1120.66 steps/s | `np.ascontiguousarray(env.screen)` baseline. |
| `step_python_grayscale` | 1257.46 steps/s | 1261.50 steps/s | NumPy integer luma conversion baseline. |
| `step_native_rgb_contiguous` | n/a | 1420.08 steps/s | Reused native RGB output buffer. |
| `step_native_grayscale` | n/a | 1403.44 steps/s | Reused native grayscale output buffer. |

On this machine, native reusable RGB copy was close to step-only and faster
than the NumPy contiguous-copy baseline. Native reusable grayscale was faster
than the NumPy grayscale baseline. Users should benchmark their own action
policy, wrapper stack, crop/resize code, and vectorization strategy before
assuming the same ranking.
