# Native Mapper Capability Gaps

The current native mapper surface supports mapper 0 (NROM), mapper 1
(SxROM/MMC1), mapper 2 (UxROM), and mapper 3 (CNROM). Characterization tests
cover the current PRG/CHR mapping, mirroring, CHR RAM, and backup/restore
behavior for those families.

The mapper implementation specs in the umbrella repository's `specs/mappers/`
queue need native capabilities that are not represented by the current
`Mapper` interface:

- CPU/PPU timing hooks for scanline counters, delayed IRQs, and mappers that
  observe A12 or other bus transitions.
- An IRQ line contract between mappers, the CPU, and the frame loop.
- Nametable mapping hooks for mapper-controlled CIRAM, VRAM, ROM nametables,
  and one-screen variants beyond the current mirroring callback.
- Expansion-area reads and writes for register ranges below `$6000`.
- Explicit PRG RAM sizing, enable/protect bits, battery-backed persistence
  metadata, and work RAM state separate from the iNES battery flag.
- Bus-conflict policy per mapper. UxROM currently takes the written value
  directly and does not emulate bus conflicts.
- NES 2.0 header and submapper metadata for mapper variants and larger ROM/RAM
  sizes.
- Mapper-owned backup/restore state for bank registers, CHR RAM, latches,
  counters, protection flags, and expansion registers.

Follow-up architecture specs:

- `specs/011-nes-py-cartridge-header-and-memory-map-model.md`
- `specs/012-nes-py-mapper-api-lifecycle-and-timing-hooks.md`
- `specs/013-nes-py-mapper-bank-helpers-and-current-mapper-cleanup.md`
