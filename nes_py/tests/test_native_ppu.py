"""Native PPU and picture-bus characterization tests."""
from unittest import TestCase

from nes_py.nes_env import _native_ppu_characterization_smoke_results


class ShouldCharacterizeNativePPUAndPictureBus(TestCase):
    def test_ppu_smoke_checks_cover_addressing_and_render_paths(self):
        results = _native_ppu_characterization_smoke_results()

        self.assertEqual({
            'pattern_table_reads_writes',
            'nametable_mirroring',
            'four_screen_mirroring',
            'one_screen_mirroring',
            'palette_mirroring',
            'address_3fff',
            'ppudata_buffer_original_address',
            'ppu_reset_latches',
            'render_hook_sequence',
        }, set(results))
        self.assertTrue(all(results.values()), results)
