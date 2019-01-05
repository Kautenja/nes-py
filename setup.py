"""The setup script for installing and distributing the nes-py package."""
import os
from glob import glob
from setuptools import setup, find_packages, Extension
from setuptools.command.build_ext import build_ext


class BuildExt(build_ext):
    """
    A build extension to omit the '-Wstrict-prototypes' flag.

    Reference:
        https://stackoverflow.com/questions/8106258/cc1plus-warning-command-line-option-wstrict-prototypes-is-valid-for-ada-c-o
    """

    def build_extensions(self):
        """Build the extensions."""
        try:
            self.compiler.compiler_so.remove("-Wstrict-prototypes")
        except (AttributeError, ValueError):
            pass
        build_ext.build_extensions(self)


# read the contents from the README file
with open('README.md') as README_file:
    README = README_file.read()


# The prefix name for the .so library to build. It will follow the format
# lib_nes_env.*.so where the * changes depending on the build system
LIB_NAME = 'nes_py.lib_nes_env'
# The source files for building the extension. Globs locate all the cpp files
# used by the LaiNES subproject. MANIFEST.in has to include the blanket
# "cpp" directory to ensure that the .inc file gets included too
SOURCES = glob('nes_py/cpp/*.cpp') + glob('nes_py/cpp/mappers/*.cpp')
# The directory pointing to header files used by the LaiNES cpp files.
# This directory has to be included using MANIFEST.in too to include the
# headers with sdist
INCLUDE_DIRS = ['nes_py/cpp/include']
# Build arguments to pass to the compiler
EXTRA_COMPILE_ARGS = ['-O2']
# add additional arguments for Unix-based systems only
if os.name != 'nt':
    EXTRA_COMPILE_ARGS += [
        '-std=c++1y',
        '-march=native',
        '-pipe',
    ]
# The official extension using the name, source, headers, and build args
LIB_NES_ENV = Extension(LIB_NAME,
    sources=SOURCES,
    include_dirs=INCLUDE_DIRS,
    extra_compile_args=EXTRA_COMPILE_ARGS,
)


setup(
    name='nes_py',
    version='4.0.0',
    description='An NES Emulator and OpenAI Gym interface',
    long_description=README,
    long_description_content_type='text/markdown',
    keywords='NES Emulator OpenAI-Gym',
    classifiers=[
        'Development Status :: 5 - Production/Stable',
        'Intended Audience :: Developers',
        'Intended Audience :: Science/Research',
        'License :: OSI Approved :: BSD License',
        'Operating System :: MacOS :: MacOS X',
        'Operating System :: POSIX :: Linux',
        'Programming Language :: C++',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
        'Topic :: Games/Entertainment',
        'Topic :: Software Development :: Libraries :: Python Modules',
        'Topic :: System :: Emulators',
    ],
    url='https://github.com/Kautenja/nes-py',
    author='Christian Kauten',
    author_email='kautencreations@gmail.com',
    license='Proprietary',
    packages=find_packages(exclude=['tests', '*.tests', '*.tests.*']),
    ext_modules=[LIB_NES_ENV],
    zip_safe=False,
    install_requires=[
        'gym>=0.10.9',
        'matplotlib>=2.3.2',
        'numpy>=1.12.1',
        'opencv-python>=3.4.0.12',
        'pygame>=1.9.3',
        'pyglet>=1.3.2',
        'tqdm>=4.19.5',
    ],
    entry_points={
        'console_scripts': [
            'nes_py = nes_py._app.cli:main',
        ],
    },
    cmdclass={'build_ext': BuildExt},
)
