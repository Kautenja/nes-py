#!/bin/bash
#
# Usage:
#     ./main.sh [options] [command] [args...]
#
# Options:
#     -h    Print documentation about this script
#
# Commands:
#     python              Print the Python interpreter used by this project
#     install             Create/use .venv and install project dependencies
#     unittest            Install the package, then execute the unit test suite
#     test                Alias for unittest
#     clean               Remove build artifacts and Python bytecode
#     deployment          Clean, then build source and wheel distributions
#     build               Alias for deployment
#     dist                Alias for deployment
#     package             Alias for deployment
#     ship                Run tests and build distributions for a release
#     release             Alias for ship
#     upload              Alias for ship
#     all                 Run tests and build distributions
#     cli                 Run the nes-py play CLI; pass extra args after it
#     play                Alias for cli
#     random              Run the nes-py play CLI in random mode
#     speedtest           Run the NES benchmark CLI
#     benchmark           Alias for speedtest
#     *                   Execute the command directly from the project root
#
# Examples:
#     ./main.sh install
#     ./main.sh test
#     ./main.sh deployment
#     ./main.sh cli --rom nes_py/tests/games/super-mario-bros-1.nes
#     ./main.sh random --rom nes_py/tests/games/super-mario-bros-1.nes --steps 100
#     ./main.sh speedtest --rom nes_py/tests/games/super-mario-bros-1.nes --steps 1000
#

set -euo pipefail

print_help() {
  sed -ne '/^#/!q;s/.\{1,2\}//;1d;p' < "$0"
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

if [[ -x "${SCRIPT_DIR}/.venv/bin/python" ]]; then
  PYTHON="${SCRIPT_DIR}/.venv/bin/python"
else
  PYTHON="${PYTHON:-python3}"
fi

while getopts ":h" optname; do
  case "$optname" in
  "h")
    print_help
    exit 0
    ;;
  "?")
    echo "Unknown option ${OPTARG}" >&2
    exit 1
    ;;
  ":")
    echo "No argument value for option ${OPTARG}" >&2
    exit 1
    ;;
  *)
    echo "Unknown error while processing options" >&2
    exit 1
    ;;
  esac
done

shift $((OPTIND - 1))

COMMAND="${1:-}"
if [[ $# -gt 0 ]]; then
  shift
fi

run_install() {
  if [[ ! -x "${SCRIPT_DIR}/.venv/bin/python" ]]; then
    echo "Creating ${SCRIPT_DIR}/.venv"
    if ! "${PYTHON}" -m venv "${SCRIPT_DIR}/.venv"; then
      echo "Failed to create .venv. Install python3-venv if your platform splits it out." >&2
      exit 1
    fi
  fi

  local venv_python="${SCRIPT_DIR}/.venv/bin/python"
  "${venv_python}" -m pip install --upgrade pip
  if [[ -f requirements.txt ]]; then
    "${venv_python}" -m pip install -r requirements.txt
  fi
  "${venv_python}" -m pip install --editable . --config-settings=editable.mode=inplace
}

run_clean() {
  rm -rf build/ dist/ .eggs/ *.egg-info/ .cmake/ CMakeFiles/ generated/
  rm -f .ninja_deps .ninja_log .skbuild-info.json CMakeCache.txt CMakeInit.txt build.ninja cmake_install.cmake
  find nes_py -type f -name "*.pyc" -exec rm -f {} +
  find nes_py -type d -name "__pycache__" -prune -exec rm -rf {} +
  find nes_py -type d -name "build" -prune -exec rm -rf {} +
  find . -maxdepth 1 -type f -name "_native*.so" -exec rm -f {} +
  find . -maxdepth 1 -type f -name "_native*.pyd" -exec rm -f {} +
  find . -maxdepth 1 -type f -name "_native*.dylib" -exec rm -f {} +
  find nes_py -maxdepth 1 -type f -name "_native*.so" -exec rm -f {} +
  find nes_py -maxdepth 1 -type f -name "_native*.pyd" -exec rm -f {} +
  find nes_py -maxdepth 1 -type f -name "_native*.dylib" -exec rm -f {} +
  find nes_py -maxdepth 1 -type f -name "lib_nes_env*" -exec rm -f {} +
}

run_unittest() {
  "${PYTHON}" -m pip install --editable . --config-settings=editable.mode=inplace
  "${PYTHON}" -m unittest discover . "$@"
}

run_deployment() {
  run_clean
  "${PYTHON}" -m build
}

run_ship() {
  run_unittest
  run_deployment
  cat <<'EOF'

Distributions are ready in dist/.
Publish releases through the "Publish to PyPI" GitHub Actions workflow by
creating a GitHub release from a version-matching tag.
EOF
}

case "${COMMAND}" in

"")
  print_help
  exit 0
  ;;

"python")
  echo "${PYTHON}"
  exit 0
  ;;

"install")
  run_install
  exit 0
  ;;

"unittest" | "test")
  run_unittest "$@"
  exit 0
  ;;

"clean")
  run_clean
  exit 0
  ;;

"deployment" | "build" | "dist" | "package")
  run_deployment
  exit 0
  ;;

"ship" | "release" | "upload")
  run_ship
  exit 0
  ;;

"all")
  run_unittest
  run_deployment
  exit 0
  ;;

"cli" | "play")
  "${PYTHON}" -m nes_py.play "$@"
  exit 0
  ;;

"random")
  "${PYTHON}" -m nes_py.play --mode random "$@"
  exit 0
  ;;

"speedtest" | "benchmark")
  "${PYTHON}" -m nes_py.speedtest "$@"
  exit 0
  ;;

*)
  "${COMMAND}" "$@"
  exit 0
  ;;

esac
