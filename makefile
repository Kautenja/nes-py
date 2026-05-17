# build everything
all: test deployment

# run the Python test suite
test:
	python3 -m pip install .
	python3 -m unittest discover .

# clean the build directory
clean:
	rm -rf build/ dist/ .eggs/ *.egg-info/ || true
	find nes_py -name "*.pyc" -delete
	find nes_py -type d -name "__pycache__" -prune -exec rm -rf {} +
	find nes_py -type d -name "build" -prune -exec rm -rf {} +
	find nes_py -maxdepth 1 -type f -name "lib_nes_env*" -delete

# build the deployment package
deployment: clean
	python3 -m build

# ship the deployment package to PyPi
ship: deployment
	python3 -m pip install ".[release]"
	python3 -m twine upload dist/*
