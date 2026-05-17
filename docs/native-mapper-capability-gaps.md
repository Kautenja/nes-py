# Native Mapper Follow-up Notes

The current native mapper surface supports mapper 0 (NROM), mapper 1
(SxROM/MMC1), mapper 2 (UxROM), and mapper 3 (CNROM). Characterization tests
cover the current PRG/CHR mapping, mirroring, CHR RAM, and backup/restore
behavior for those families.

Specs 011-013 in the umbrella repository added the core mapper architecture
needed by the mapper queue: RAII mapper ownership, mapper cloning for
backup/restore, CPU IRQ callbacks, CPU/PPU timing hooks, expansion-area routing,
PRG RAM hooks, nametable delegation, mirroring synchronization, bus-conflict
helpers, and shared PRG/CHR bank helpers.

The mapper implementation specs in the umbrella repository's `specs/mappers/`
queue should now treat the following as remaining per-mapper work rather than
missing core API surface:

- Mapper-specific register decoding, PRG/CHR banking, mirroring, IRQ counters,
  latches, nametable mapping, expansion registers, PRG RAM protection, and
  variant behavior.
- Bus-conflict policy per mapper. UxROM currently takes the written value
  directly and does not emulate bus conflicts.
- NES 2.0 header and submapper metadata for mapper variants and larger ROM/RAM
  sizes.
- Mapper-owned backup/restore state for each new mapper's bank registers, CHR
  RAM, latches, counters, protection flags, expansion registers, and any
  mapper-provided nametable memory.
- Expansion audio register behavior where needed for compatibility. Audio output
  mixing is not part of the current mapper API and should be documented as a
  deliberate limitation unless a future audio spec adds it.
- Representative-fixture integration tests using legally supplied local ROMs,
  plus mapper-level synthetic tests that remain runnable without commercial ROMs.

Follow-up architecture specs:

- `specs/archive/011-nes-py-cartridge-header-and-memory-map-model.md`
- `specs/archive/012-nes-py-mapper-api-lifecycle-and-timing-hooks.md`
- `specs/archive/013-nes-py-mapper-bank-helpers-and-current-mapper-cleanup.md`
