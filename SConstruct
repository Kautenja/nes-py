"""The compilation script for this project using SCons."""
from os import environ


VariantDir('build/src', 'src', duplicate=0)
VariantDir('build/lib', 'lib', duplicate=0)
flags = ['-O3', '-march=native', '-std=c++14', '-lpython2.7']


env = Environment(
    ENV = environ,
    CXX = 'clang++',
    CPPFLAGS = ['-Wno-unused-value'],
    CXXFLAGS = flags,
    LINKFLAGS = flags,
    CPPPATH = ['#src/include', '/usr/include/python2.7/'],
    LIBS = ['SDL2', 'SDL2_image']
)


env.Program('build/laines', Glob('build/*/*.cpp') + Glob('build/*/*/*.cpp'))
