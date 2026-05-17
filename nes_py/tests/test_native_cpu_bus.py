"""Native CPU and main-bus characterization tests."""
from unittest import TestCase

from nes_py.nes_env import _native_cpu_characterization_smoke_results
from nes_py.nes_env import _native_main_bus_characterization_smoke_results


class ShouldCharacterizeNativeCPU(TestCase):
    def test_cpu_smoke_checks_cover_timing_sensitive_paths(self):
        results = _native_cpu_characterization_smoke_results()

        self.assertEqual({
            'reset_vector',
            'stack_push_pop',
            'addressing_modes',
            'branch_page_crossing',
            'interrupt_entry',
            'dma_cycle_skipping',
            'flag_behavior',
        }, set(results))
        self.assertTrue(all(results.values()), results)


class ShouldCharacterizeNativeMainBus(TestCase):
    def test_main_bus_smoke_checks_cover_fixed_and_mapper_paths(self):
        results = _native_main_bus_characterization_smoke_results()

        self.assertEqual({
            'ram_mirroring',
            'ppu_register_mirroring',
            'controller_reads',
            'oam_dma_page_access',
            'expansion_area',
            'prg_ram_access',
            'mapper_prg_access',
        }, set(results))
        self.assertTrue(all(results.values()), results)
