"""Native mapper lifecycle and hook smoke tests."""

from nes_py.nes_env import _native_mapper_bank_helper_smoke_results
from nes_py.nes_env import _native_mapper_hook_smoke_results

from nes_py.tests.mappers.common import MapperTestCase


class ShouldExposeMapperLifecycleAndTimingHooks(MapperTestCase):
    """Exercise native fake mappers that use the extension points."""

    def test_native_mapper_hook_smoke_tests(self):
        results = _native_mapper_hook_smoke_results()

        self.assertEqual({
            'irq',
            'cpu_cycle',
            'ppu',
            'expansion',
            'prg_ram',
            'nametable',
        }, set(results))
        self.assertTrue(all(results.values()), results)

    def test_native_mapper_bank_helper_smoke_tests(self):
        results = _native_mapper_bank_helper_smoke_results()

        self.assertEqual({
            'prg_8k',
            'prg_16k',
            'prg_32k',
            'chr_1k',
            'chr_2k',
            'chr_4k',
            'chr_8k',
            'masks_and_bus_conflicts',
        }, set(results))
        self.assertTrue(all(results.values()), results)
