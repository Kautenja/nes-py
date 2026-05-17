"""Test application imports avoid initializing optional render backends."""
import subprocess
import sys
from unittest import TestCase


class AppImportTest(TestCase):
    """Test import-time behavior for application helpers."""

    def test_cli_import_does_not_initialize_pyglet(self):
        code = """
import sys
import nes_py

if 'pyglet' in sys.modules:
    raise SystemExit('import nes_py initialized pyglet')

from nes_py.app import cli

if 'pyglet' in sys.modules:
    raise SystemExit('import nes_py.app.cli initialized pyglet')

from nes_py.app.play_human import play_human

if 'pyglet' in sys.modules:
    raise SystemExit('import play_human initialized pyglet')
"""
        completed = subprocess.run(
            [sys.executable, '-c', code],
            check=False,
            stderr=subprocess.PIPE,
            stdout=subprocess.PIPE,
            text=True,
        )
        self.assertEqual(completed.returncode, 0, completed.stderr)
