"""Test cases for the packaged speedtest benchmark."""
import contextlib
import io
import json
from unittest import TestCase

from .rom_file_abs_path import rom_file_abs_path
from nes_py.speedtest import BenchmarkConfig
from nes_py.speedtest import main
from nes_py.speedtest import run_benchmark


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
