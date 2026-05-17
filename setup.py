"""Native extension build helper for nes-py.

Static project metadata lives in ``pyproject.toml``. This file only keeps the
platform-specific extension configuration that setuptools cannot express
cleanly without custom build hooks.
"""
from pathlib import Path

from setuptools import Extension
from setuptools import setup
from setuptools.command.build_ext import build_ext


# The prefix name for the .so library to build. It will follow the format
# lib_nes_env.*.so where the * changes depending on the build system
LIB_NAME = 'nes_py.lib_nes_env'
NATIVE_ROOT = Path('nes_py') / 'nes'


def _native_sources():
    """Return native C++ sources in a stable order for reproducible builds."""
    return sorted(
        str(path)
        for path in (NATIVE_ROOT / 'src').rglob('*.cpp')
    )


# The directory pointing to header files used by the LaiNES cpp files.
INCLUDE_DIRS = ['nes_py/nes/include']
# The official extension using the name, source, headers, and build args
LIB_NES_ENV = Extension(LIB_NAME,
    sources=_native_sources(),
    include_dirs=INCLUDE_DIRS,
    language='c++',
)


class BuildExt(build_ext):
    """Select compiler flags that match the active platform toolchain."""

    C_OPTS = {
        'msvc': ['/std:c++14', '/O2'],
        'unix': ['-std=c++14', '-O3', '-pipe'],
    }

    def build_extensions(self):
        compile_args = self.C_OPTS.get(self.compiler.compiler_type, [])
        for extension in self.extensions:
            extension.extra_compile_args = compile_args
        super().build_extensions()


setup(
    ext_modules=[LIB_NES_ENV],
    cmdclass={'build_ext': BuildExt},
)
