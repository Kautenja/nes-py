# Mapper Direct Read Fast Paths

Spec 042 added mapper-provided direct read pages for common mapper PRG/CHR
windows. `MainBus` caches four 8 KiB PRG-ROM pages for CPU `$8000-$ffff`
reads, and `PictureBus` caches eight 1 KiB CHR pages for PPU `$0000-$1fff`
reads. Mapper writes refresh both caches so bank-register changes are visible
before the next bus read.

The direct CHR cache is disabled when mapper-visible PPU address/read/write
hooks or mapper nametable mapping are active. Expansion routing and PRG RAM
continue to use the existing mapper methods. CHR RAM writes are still routed
through `writeCHR`; the direct read pages point at the same CHR RAM storage
when the mapper can provide a contiguous page safely.

Rejected design: looking up a direct pointer through a virtual mapper method on
every CPU/PPU read was not kept because it preserved the virtual dispatch cost
this spec was intended to remove. Bus-side page caches keep the address hot path
to an indexed pointer read while retaining mapper ownership of bank selection.

## Commands

```sh
build/nes-emu-release/nes_emu_benchmarks --benchmark-samples 1 --benchmark-resamples 1
.venv/bin/python -m nes_py.speedtest --rom nes_py/tests/games/super-mario-bros-1.nes --steps 1000 --warmup-steps 100 --json --no-progress
.venv/bin/python -m nes_py.speedtest --rom nes_py/tests/games/the-legend-of-zelda.nes --steps 1000 --warmup-steps 100 --json --no-progress
```

The PRG-read baseline was captured from commit `a29f6b3` in a temporary
worktree with only the new PRG benchmark harness applied, so it measured the old
virtual `readPRG` path with the same benchmark body.

## Native Mapper Read Stress

| Profile | Before | After | Change |
| --- | ---: | ---: | ---: |
| NROM PRG reads | 25.318 us | 18.651 us | -26.3% |
| SxROM PRG reads | 26.109 us | 18.693 us | -28.4% |
| UxROM PRG reads | 25.776 us | 18.818 us | -27.0% |
| CNROM PRG reads | 25.193 us | 19.442 us | -22.8% |
| NROM CHR reads | 20.526 us | 13.680 us | -33.4% |
| SxROM CHR reads | 24.568 us | 12.992 us | -47.1% |
| UxROM CHR reads | 18.388 us | 13.221 us | -28.1% |
| CNROM CHR reads | 20.900 us | 12.784 us | -38.8% |

## Representative Frames

| ROM | Mapper | Before | After | Change |
| --- | ---: | ---: | ---: | ---: |
| `super-mario-bros-1.nes` | 0 | 720.609 us | 732.734 us | +1.7% |
| `the-legend-of-zelda.nes` | 1 | 750.026 us | 742.651 us | -1.0% |
| `mega-man.nes` | 2 | 426.234 us | 430.692 us | +1.0% |
| `adventure-island.nes` | 3 | 416.942 us | 396.067 us | -5.0% |

## Public Speedtest

| ROM | Before | After | Change |
| --- | ---: | ---: | ---: |
| `super-mario-bros-1.nes` | 1353.26 steps/s | 1495.08 steps/s | +10.5% |
| `the-legend-of-zelda.nes` | 1478.61 steps/s | 1536.16 steps/s | +3.9% |

The benchmark suite uses one Catch2 sample and one resample to match Ralph
verification. Treat the full-frame rows as smoke-profile direction rather than a
strict threshold; the read-stress rows isolate the intended fast path.
