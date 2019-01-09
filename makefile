# an alias to the python command
PYTHON=python

# build the LaiNES code, test the Python interface, and build
# the deployment package
all: test deployment

#
# MARK: Development
#

# build the LaiNES CPP code
lib_nes_env:
	cd nes_py/nes && scons
	mv nes_py/nes/lib_nes_env*.so nes_py

# run the Python test suite
test: lib_nes_env
	${PYTHON} -m unittest discover .

#
# MARK: Deployment
#

# clean the build directory
clean:
	rm -rf build/ dist/ .eggs/ *.egg-info/ || true
	rm -rf nes_py/cpp/build || true
	rm nes_py/lib_nes_env.so || true
	find . -name "*.pyc" -delete
	find . -name "__pycache__" -delete
	find . -name ".sconsign.dblite" -delete

# build the deployment package
deployment: clean
	${PYTHON} setup.py sdist bdist_wheel --universal

# ship the deployment package to PyPi
ship: test deployment
	twine upload dist/*
