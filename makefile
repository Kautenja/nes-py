# an alias to the python command
PYTHON=python3

# build the LaiNES code, test the Python interface, and build
# the deployment package
all: test deployment

#
# MARK: Development
#

# build the LaiNES CPP code
lib_nes_env:
	scons -C nes_py/nes
	mv nes_py/nes/lib_nes_env*.so nes_py

# run the Python test suite
test: lib_nes_env
	${PYTHON} -m unittest discover .

#
# MARK: Deployment
#

clean_dist:
	rm -rf build/ dist/ .eggs/ *.egg-info/ || true

clean_python_build:
	find . -name "*.pyc" -delete
	find . -name "__pycache__" -delete

clean_cpp_build:
	find . -name ".sconsign.dblite" -delete
	find . -name "build" | rm -rf
	find . -name "lib_nes_env.so" -delete

# clean the build directory
clean: clean_dist clean_python_build clean_cpp_build

# build the deployment package
deployment: clean
	${PYTHON} setup.py sdist bdist_wheel --universal

# ship the deployment package to PyPi
ship: test deployment
	twine upload dist/*
