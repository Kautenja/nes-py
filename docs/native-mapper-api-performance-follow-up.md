# Native Mapper API Performance Follow-Up

Spec 012 added mapper lifecycle ownership, state snapshots, IRQ callbacks, CPU/PPU hooks, expansion routing, PRG RAM routing, and nametable delegation. A tiny mapper profile was run before and after the refactor on macOS arm64, CPython 3.14.2, clang, with `--steps 20 --warmup-steps 5`.

## Tiny Profile Comparison

| Mapper | Operation | Before steps/s | After steps/s | Note |
| --- | ---: | ---: | ---: | --- |
| 0 | reset | 2,711,875.86 | 2,307,602.41 | Tiny timing, noisy |
| 0 | step | 2,176.72 | 2,167.99 | No meaningful regression observed |
| 0 | render_rgb_array | 10,433,224.54 | 10,666,751.01 | No regression observed |
| 0 | backup_restore | 87,639.34 | 63,066.61 | Snapshot now includes mapper PRG RAM and PPU screen state |
| 1 | reset | 2,608,583.37 | 2,436,653.50 | Tiny timing, noisy |
| 1 | step | 1,178.10 | 1,101.74 | Small hot-path cost remains |
| 1 | render_rgb_array | 10,910,970.87 | 10,666,419.88 | Tiny timing, noisy |
| 1 | backup_restore | 77,382.02 | 75,187.97 | Small snapshot cost remains |

## Follow-Up

The remaining step-throughput cost is expected to live in the generic `PictureBus::read/write` hook checks and the per-CPU-cycle mirroring synchronization check. Before adding many new mapper implementations, profile a fast path for mappers without PPU hooks or nametable mapping, or split hooked and unhooked picture-bus accessors so NROM/SxROM/UxROM/CNROM keep their previous hot path.
