"""A CTypes interface to the C++ NES environment."""
from ctypes import cdll, c_wchar_p


# load the library from the shared object file
lib = cdll.LoadLibrary('build/lib_nes_env.so')


class NesENV(object):
    """An NES environment based on the LaiNES emulator."""

    def __init__(self):
        """Create a new LaiNES environment."""
        self.obj = lib.Foo_new()

    def bar(self, path):
        lib.Foo_bar(self.obj, c_wchar_p(path))


f = NesENV()
f.bar('/Users/jameskauten/Desktop/super-mario-bros-1.nes')
