"""Test play imports avoid initializing optional render backends."""
import subprocess
import sys
from unittest import TestCase


class PlayImportTest(TestCase):
    """Test import-time behavior for play helpers."""

    def test_play_import_does_not_initialize_pyglet(self):
        code = """
import sys
import nes_py

if 'pyglet' in sys.modules:
    raise SystemExit('import nes_py initialized pyglet')

from nes_py import play

if 'pyglet' in sys.modules:
    raise SystemExit('import nes_py.play initialized pyglet')

_ = play.play_human

if 'pyglet' in sys.modules:
    raise SystemExit('accessing play_human initialized pyglet')
"""
        completed = subprocess.run(
            [sys.executable, '-c', code],
            check=False,
            stderr=subprocess.PIPE,
            stdout=subprocess.PIPE,
            text=True,
        )
        self.assertEqual(completed.returncode, 0, completed.stderr)
