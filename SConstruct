"""The compilation script for this project using SCons."""
from os import environ


VariantDir('nes_py/laines/build/src', 'nes_py/laines', duplicate=0)
flags = ['-O3', '-march=native', '-std=c++14']


env = Environment(
    ENV = environ,
    CXX = 'clang++',
    CPPFLAGS = ['-Wno-unused-value'],
    CXXFLAGS = flags,
    LINKFLAGS = flags,
    CPPPATH = ['#nes_py/laines/include'],
)


# Compile the shared library for the Python interface
source_files = Glob('nes_py/laines/build/*/*.cpp') + Glob('nes_py/laines/build/*/*/*.cpp')
env.SharedLibrary('nes_py/laines/build/_nes_env.so', source_files)
