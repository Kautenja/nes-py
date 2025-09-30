# Changelog

## 9.0.0 - 2025-09-30

### Breaking Changes

- Migrated from `gym` to `gymnasium`.
- The `reset` method now returns a tuple of `(obs, info)` instead of just `obs`.
- The `step` method now returns a 5-element tuple `(obs, reward, terminated, truncated, info)` instead of a 4-element tuple.
- The `seed` method has been removed. Seeding is now handled by `reset(seed=...)`.
- Updated dependencies: `numpy`, `pyglet`, `tqdm`.
- Dropped support for Python < 3.9.

### Added

- Support for Python 3.9, 3.10, 3.11, and 3.12.
