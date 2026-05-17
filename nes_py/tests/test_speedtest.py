"""Test cases for the packaged speedtest benchmark."""
import contextlib
import io
import json
import tempfile
from unittest import TestCase

from nes_py.tests.mapper_fixtures import synthetic_rom_path
from nes_py.tests.rom_file_abs_path import rom_file_abs_path
from nes_py.speedtest import BenchmarkConfig
from nes_py.speedtest import main
from nes_py.speedtest import run_benchmark
from nes_py.speedtest import run_mapper_profile


class ShouldRunBenchmark(TestCase):
    def test_returns_structured_result(self):
        result = run_benchmark(BenchmarkConfig(
            rom=rom_file_abs_path('super-mario-bros-1.nes'),
            steps=5,
            warmup_steps=2,
            seed=1,
            progress=False,
            backup_interval=3,
            restore_interval=4,
        ))

        self.assertEqual(7, result.total_steps)
        self.assertEqual(5, result.measured_steps)
        self.assertEqual(2, result.warmup_steps)
        self.assertGreaterEqual(result.resets, 1)
        self.assertGreaterEqual(result.backups, 1)
        self.assertGreaterEqual(result.restores, 1)
        self.assertGreater(result.elapsed_seconds, 0)
        self.assertGreater(result.steps_per_second, 0)
        self.assertGreater(result.frames_per_second, 0)
        self.assertEqual(1, result.seed)
        self.assertEqual('random', result.action_policy)
        self.assertEqual(3, result.backup_interval)
        self.assertEqual(4, result.restore_interval)

    def test_runs_against_synthetic_rom(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = synthetic_rom_path(
                tmpdir,
                'nrom.nes',
                mapper=0,
                prg_banks=1,
                chr_banks=1,
            )

            result = run_benchmark(BenchmarkConfig(
                rom=path,
                steps=2,
                warmup_steps=1,
                action_policy='noop',
                progress=False,
            ))

        self.assertEqual(3, result.total_steps)
        self.assertEqual(2, result.measured_steps)
        self.assertEqual('noop', result.action_policy)
        self.assertGreater(result.steps_per_second, 0)


class ShouldRunBenchmarkCLI(TestCase):
    def test_json_output(self):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            status = main([
                '--rom',
                rom_file_abs_path('super-mario-bros-1.nes'),
                '--steps',
                '3',
                '--warmup-steps',
                '1',
                '--seed',
                '1',
                '--backup-interval',
                '2',
                '--restore-interval',
                '3',
                '--json',
                '--no-progress',
            ])

        self.assertEqual(0, status)
        data = json.loads(output.getvalue())
        self.assertEqual(4, data['total_steps'])
        self.assertEqual(3, data['measured_steps'])
        self.assertEqual(1, data['warmup_steps'])
        self.assertEqual(2, data['backup_interval'])
        self.assertEqual(3, data['restore_interval'])
        self.assertGreater(data['elapsed_seconds'], 0)
        self.assertGreater(data['steps_per_second'], 0)


class ShouldRunCurrentMapperBenchmarkProfile(TestCase):
    def test_returns_shape_and_positive_timings(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            nrom = synthetic_rom_path(
                tmpdir,
                'nrom.nes',
                mapper=0,
                prg_banks=1,
                chr_banks=1,
            )
            sxrom = synthetic_rom_path(
                tmpdir,
                'sxrom.nes',
                mapper=1,
                prg_banks=4,
                chr_banks=0,
            )

            results = run_mapper_profile(
                [nrom, sxrom],
                steps=2,
                warmup_steps=1,
            )

        self.assertEqual(8, len(results))
        seen = {(result.mapper, result.operation) for result in results}
        self.assertEqual({
            (0, 'reset'),
            (0, 'step'),
            (0, 'render_rgb_array'),
            (0, 'backup_restore'),
            (1, 'reset'),
            (1, 'step'),
            (1, 'render_rgb_array'),
            (1, 'backup_restore'),
        }, seen)
        for result in results:
            data = result.to_dict()
            self.assertIn('environment', data)
            self.assertIn('compiler', data)
            self.assertIn('platform', data)
            self.assertIn('mapper', data)
            self.assertIn('operation', data)
            self.assertGreater(data['elapsed_seconds'], 0)
            self.assertGreater(data['steps_per_second'], 0)

    def test_cli_json_output(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            nrom = synthetic_rom_path(
                tmpdir,
                'nrom.nes',
                mapper=0,
                prg_banks=1,
                chr_banks=1,
            )
            sxrom = synthetic_rom_path(
                tmpdir,
                'sxrom.nes',
                mapper=1,
                prg_banks=4,
                chr_banks=0,
            )

            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                status = main([
                    '--profile-rom',
                    nrom,
                    '--profile-rom',
                    sxrom,
                    '--steps',
                    '1',
                    '--warmup-steps',
                    '0',
                    '--json',
                    '--no-progress',
                ])

        self.assertEqual(0, status)
        data = json.loads(output.getvalue())
        self.assertEqual(8, len(data))
        for result in data:
            self.assertIn(result['mapper'], {0, 1})
            self.assertGreater(result['elapsed_seconds'], 0)
            self.assertGreater(result['steps_per_second'], 0)
