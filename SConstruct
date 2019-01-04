"""The compilation script for this project using SCons."""
from os import environ


VariantDir('nes_py/cpp/build/src', 'nes_py/cpp', duplicate=0)


FLAGS = [
    '-std=c++1y',
    '-O2',
    '-march=native',
    '-pipe',
]


ENV = Environment(
    ENV=environ,
    CXX='clang++',
    CPPFLAGS=['-Wno-unused-value'],
    CXXFLAGS=FLAGS,
    LINKFLAGS=FLAGS,
    CPPPATH=['#nes_py/cpp/include'],
)
# TODO: Remove when SFML is removed from dependency tree
ENV.Append(LIBS = ["sfml-graphics","sfml-window","sfml-system"]);
ENV.Append(LIBPATH = "/usr/local/lib");


# Compile the shared library for the Python interface
CPP = Glob('nes_py/cpp/build/*/*.cpp') + Glob('nes_py/cpp/build/*/*/*.cpp')
ENV.SharedLibrary('nes_py/_nes_env.so', CPP)
