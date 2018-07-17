"""A CTypes interface to the C++ NES environment."""
from ctypes import cdll


# load the library from the shared object file
lib = cdll.LoadLibrary('build/lib_nes_env.so')


class NesENV(object):
    """An NES environment based on the LaiNES emulator."""

    def __init__(self):
        """Create a new LaiNES environment."""
        self.obj = lib.Foo_new()

    def bar(self):
        lib.Foo_bar(self.obj)


f = NesENV()
f.bar() #and you will see "Hello" on the screen
