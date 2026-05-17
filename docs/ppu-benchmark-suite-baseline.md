# PPU Benchmark Suite Baseline

Captured on 2026-05-17 on macOS 26.3 arm64 with CPython 3.14.2,
Apple clang 21.0.0, and the native benchmark target built in Release mode.

## Commands

```sh
.venv/bin/python -m pip install -e .
cmake -S . -B build/nes-emu-release -DCMAKE_BUILD_TYPE=Release -DNES_EMU_BUILD_BENCHMARKS=ON
cmake --build build/nes-emu-release --config Release --target nes_emu_benchmarks
build/nes-emu-release/nes_emu_benchmarks --benchmark-samples 1 --benchmark-resamples 1
.venv/bin/python -m nes_py.speedtest --rom nes_py/tests/games/super-mario-bros-1.nes --steps 1000 --warmup-steps 100 --json --no-progress
.venv/bin/python -m nes_py.speedtest --rom nes_py/tests/games/the-legend-of-zelda.nes --steps 1000 --warmup-steps 100 --json --no-progress
```

## Native PPU Profiles

The synthetic PPU profiles use fixed pattern-table, nametable, attribute, and
palette data. The ROM frame profiles restore a captured emulator state before
each measured frame, so a before/after comparison on the same machine starts
from the same CPU, PPU, bus, and mapper state.

The command above ran with one Catch2 sample and one resample to match the
Ralph verification path. Use more samples for local profiling when comparing
small changes.

| Profile | Mapper | Render mode | Operation | Mean |
| --- | ---: | --- | --- | ---: |
| synthetic render-off | 0 | mask-off | one PPU frame | 284.151 us |
| synthetic background-only | 0 | background | one PPU frame | 450.984 us |
| synthetic sprite-heavy | 0 | background+sprites | one PPU frame | 577.359 us |
| synthetic NROM CHR reads | 0 | CHR read stress | 8,192 picture-bus reads | 19.192 us |
| synthetic SxROM CHR reads | 1 | CHR read stress | 8,192 picture-bus reads | 21.109 us |
| synthetic UxROM CHR reads | 2 | CHR read stress | 8,192 picture-bus reads | 17.013 us |
| synthetic CNROM CHR reads | 3 | CHR read stress | 8,192 picture-bus reads | 17.317 us |
| `super-mario-bros-1.nes` | 0 | full-frame | restore and step one frame | 724.984 us |
| `the-legend-of-zelda.nes` | 1 | full-frame | restore and step one frame | 767.609 us |
| `mega-man.nes` | 2 | full-frame | restore and step one frame | 418.317 us |
| `adventure-island.nes` | 3 | full-frame | restore and step one frame | 413.401 us |

## Public Speedtest Comparison

These numbers use the supported `nes_py.speedtest` CLI rather than native test
helpers.

| ROM | Steps | Warmup | Action policy | Render mode | Steps/s |
| --- | ---: | ---: | --- | --- | ---: |
| `super-mario-bros-1.nes` | 1,000 | 100 | random | none | 1305.15 |
| `the-legend-of-zelda.nes` | 1,000 | 100 | random | none | 1470.35 |

## Notes

- Native benchmark labels include the mapper, ROM or synthetic scenario,
  render mode, and measured operation.
- The suite is intentionally threshold-free; compare before/after runs on the
  same host rather than treating these machine-local timings as CI gates.
- No generated benchmark output is required or expected in the repository.
