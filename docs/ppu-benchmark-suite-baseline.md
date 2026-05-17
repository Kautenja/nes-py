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

Mapper CHR read and representative ROM profiles cover every mapper currently
registered by the native mapper factory: 0, 1, 2, 3, 4, 5, 7, 9, and 69.

The command above ran with one Catch2 sample and one resample to match the
Ralph verification path. Use more samples for local profiling when comparing
small changes.

| Profile | Mapper | Render mode | Operation | Mean |
| --- | ---: | --- | --- | ---: |
| synthetic render-off | 0 | mask-off | one PPU frame | 290.567 us |
| synthetic background-only | 0 | background | one PPU frame | 450.318 us |
| synthetic sprite-heavy | 0 | background+sprites | one PPU frame | 603.734 us |
| synthetic NROM CHR reads | 0 | CHR read stress | 8,192 picture-bus reads | 20.109 us |
| synthetic SxROM CHR reads | 1 | CHR read stress | 8,192 picture-bus reads | 22.276 us |
| synthetic UxROM CHR reads | 2 | CHR read stress | 8,192 picture-bus reads | 16.784 us |
| synthetic CNROM CHR reads | 3 | CHR read stress | 8,192 picture-bus reads | 18.026 us |
| synthetic MMC3 CHR reads | 4 | CHR read stress | 8,192 picture-bus reads | 152.068 us |
| synthetic MMC5 CHR reads | 5 | CHR read stress | 8,192 picture-bus reads | 29.650 us |
| synthetic AxROM CHR reads | 7 | CHR read stress | 8,192 picture-bus reads | 17.013 us |
| synthetic MMC2 CHR reads | 9 | CHR read stress | 8,192 picture-bus reads | 40.568 us |
| synthetic FME-7 CHR reads | 69 | CHR read stress | 8,192 picture-bus reads | 21.775 us |
| `super-mario-bros-1.nes` | 0 | full-frame | restore and step one frame | 732.568 us |
| `the-legend-of-zelda.nes` | 1 | full-frame | restore and step one frame | 772.025 us |
| `mega-man.nes` | 2 | full-frame | restore and step one frame | 412.026 us |
| `adventure-island.nes` | 3 | full-frame | restore and step one frame | 417.609 us |
| `super-mario-bros-3.nes` | 4 | full-frame | restore and step one frame | 430.525 us |
| `castlevania-iii-draculas-curse.nes` | 5 | full-frame | restore and step one frame | 487.442 us |
| `battletoads.nes` | 7 | full-frame | restore and step one frame | 409.067 us |
| `mike-tysons-punch-out.nes` | 9 | full-frame | restore and step one frame | 494.568 us |
| `batman-return-of-the-joker.nes` | 69 | full-frame | restore and step one frame | 770.734 us |

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
