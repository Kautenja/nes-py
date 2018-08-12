#
# MARK: Globals
#

# the path to the python interpreter
PYTHON=python3

# build the LaiNES code, test the Python interface, and build 
# the deployment package
all: test deployment

#
# MARK: Development
#

# build the LaiNES CPP code
laines:
	scons

# run the Python test suite
test: laines
	${PYTHON} -m unittest discover .

#
# MARK: Deployment
#

# clean the build directory
clean:
	rm -rf build/ dist/ .eggs/ *.egg-info/ || true

# build the deployment package
deployment: clean
	${PYTHON} setup.py sdist bdist_wheel --universal

# ship the deployment package to PyPi
ship: test deployment
	twine upload dist/*
