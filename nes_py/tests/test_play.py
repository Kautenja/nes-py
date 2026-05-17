"""Test cases for the packaged play command."""
import contextlib
import io
from unittest import TestCase

from .rom_file_abs_path import rom_file_abs_path
from nes_py.play import main


class ShouldRunPlayCLI(TestCase):
    """Test the play command line interface."""

    def test_random_no_render_output(self):
        status = main([
            '--rom',
            rom_file_abs_path('super-mario-bros-1.nes'),
            '--mode',
            'random',
            '--steps',
            '3',
            '--no-render',
            '--no-progress',
        ])

        self.assertEqual(0, status)

    def test_human_requires_rendering(self):
        stderr = io.StringIO()
        with contextlib.redirect_stderr(stderr):
            with self.assertRaises(SystemExit) as context:
                main([
                    '--rom',
                    rom_file_abs_path('super-mario-bros-1.nes'),
                    '--mode',
                    'human',
                    '--no-render',
                ])

        self.assertEqual(2, context.exception.code)
