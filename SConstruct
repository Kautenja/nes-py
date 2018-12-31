"""The compilation script for this project using SCons."""
from os import environ


VariantDir('nes_py/cpp/build/src', 'nes_py/cpp', duplicate=0)
flags = ['-O3', '-march=native', '-std=c++1y']


env = Environment(
    ENV = environ,
    CXX = 'clang++',
    CPPFLAGS = ['-Wno-unused-value'],
    CXXFLAGS = flags,
    LINKFLAGS = flags,
    CPPPATH = ['#nes_py/cpp/include'],
)


# Compile the shared library for the Python interface
source_files = Glob('nes_py/cpp/build/*/*.cpp') + Glob('nes_py/cpp/build/*/*/*.cpp')
env.SharedLibrary('nes_py/_nes_env.so', source_files)
