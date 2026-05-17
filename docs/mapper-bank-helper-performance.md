# Mapper Bank Helper Performance

Spec 013 refactored mappers 0-3 onto small shared PRG/CHR bank window
helpers. The comparison below was run on macOS arm64 with CPython 3.14.2
from a local editable install in `.venv`.

Commands:

```sh
.venv/bin/python -m nes_py.speedtest --rom nes_py/tests/games/super-mario-bros-1.nes --steps 100 --warmup-steps 20 --json --no-progress
.venv/bin/python -m nes_py.speedtest --rom nes_py/tests/games/the-legend-of-zelda.nes --steps 100 --warmup-steps 20 --json --no-progress
```

| ROM | Before steps/s | After steps/s | Change |
| --- | ---: | ---: | ---: |
| super-mario-bros-1.nes | 879.77 | 896.74 | +1.9% |
| the-legend-of-zelda.nes | 1301.62 | 1480.66 | +13.8% |

The helper refactor did not show a regression in these focused smoke runs.
