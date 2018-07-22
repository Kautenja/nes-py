"""The setup script for installing and distributing the nes-py package."""
from glob import glob
from setuptools import setup, find_packages, Extension


def README():
    """Return the contents of the README file for this project."""
    with open('README.md') as README_file:
        return README_file.read()


# The prefix name for the .so library to build. It will follow the format
# lib_nes_env.*.so where the * changes depending on the build system
lib_name = 'nes_py/laines/build/lib_nes_env'
# The source files for building the extension. Globs locate all the cpp files
# used by the LaiNES subproject. MANIFEST.in has to include the blanket
# "laines" directory to ensure that the .inc file gets included too
cpp = glob('nes_py/laines/*.cpp') + glob('nes_py/laines/mappers/*.cpp')
# The directory pointing to header files used by the LaiNES cpp files.
# This directory has to be included using MANIFEST.in too to include the
# headers with sdist
hpp = ['nes_py/laines/include']
# Additional build arguments to pass to the compiler
compile_args = ['-O3', '-march=native', '-std=c++1y']
# The official extension using the name, source, headers, and build args
lib_nes_env = Extension(lib_name,
    sources=cpp,
    include_dirs=hpp,
    extra_compile_args=compile_args,
)


setup(
    name='nes_py',
    version='0.8.5',
    description='An NES Emulator and OpenAI Gym interface',
    long_description=README(),
    long_description_content_type='text/markdown',
    keywords='NES Emulator OpenAI-Gym',
    classifiers=[
        'Development Status :: 4 - Beta',
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
    license='BSD-2-Clause',
    packages=find_packages(exclude=['tests', '*.tests', '*.tests.*']),
    ext_modules=[lib_nes_env],
    zip_safe=False,
    install_requires=[
        'gym>=0.10.5',
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
)
