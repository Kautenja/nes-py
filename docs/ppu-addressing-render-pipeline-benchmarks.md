# PPU Addressing and Render Pipeline Benchmarks

Captured on 2026-05-17 on macOS arm64 with CPython 3.14.2, clang, and the
packaged `nes_py.speedtest` CLI. Commands used 1,000 measured steps and 100
warmup steps.

## Normal Step Profile

| ROM | Before steps/s | After steps/s | Change |
| --- | ---: | ---: | ---: |
| `super-mario-bros-1.nes` | 856.45 | 1262.97 | +47.47% |
| `the-legend-of-zelda.nes` | 932.78 | 1438.24 | +54.19% |

## Render-Heavy Profile

The render-heavy profile used `--render-mode rgb_array`, forcing every measured
step to read the RGB observation path.

| ROM | Before frames/s | After frames/s | Change |
| --- | ---: | ---: | ---: |
| `super-mario-bros-1.nes` | 842.50 | 1287.56 | +52.83% |
| `the-legend-of-zelda.nes` | 901.92 | 1459.13 | +61.78% |

The speedup comes from normalizing picture-bus address decoding once per access,
using fixed-size PPU/PictureBus storage, avoiding per-scanline sprite vector
resize/push operations, and caching background tile-row and attribute bytes when
the active mapper has no mapper-visible PPU hooks.
