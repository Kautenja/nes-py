cmake_minimum_required(VERSION 3.21...4.2)

project(${SKBUILD_PROJECT_NAME} VERSION ${SKBUILD_PROJECT_VERSION} LANGUAGES CXX)

find_package(Python COMPONENTS Interpreter Development.Module NumPy REQUIRED)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(NES_PY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/nes_py")
set(NES_NATIVE_ROOT "${NES_PY_ROOT}/nes")
set(NES_CYTHON_SOURCE "${NES_PY_ROOT}/_native.pyx")
set(NES_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(NES_GENERATED_SOURCE "${NES_GENERATED_DIR}/_native.cpp")

add_custom_command(
    OUTPUT "${NES_GENERATED_SOURCE}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${NES_GENERATED_DIR}"
    COMMAND "${Python_EXECUTABLE}" -m cython --cplus -3 -o "${NES_GENERATED_SOURCE}" "${NES_CYTHON_SOURCE}"
    DEPENDS "${NES_CYTHON_SOURCE}"
    COMMENT "Cythonizing nes_py._native"
    VERBATIM
)

file(GLOB_RECURSE NES_NATIVE_SOURCES CONFIGURE_DEPENDS
    "${NES_NATIVE_ROOT}/src/*.cpp"
)

Python_add_library(_native MODULE WITH_SOABI
    "${NES_GENERATED_SOURCE}"
    ${NES_NATIVE_SOURCES}
)

target_include_directories(_native PRIVATE
    "${NES_NATIVE_ROOT}/include"
    "${Python_NumPy_INCLUDE_DIRS}"
)

target_compile_options(_native PRIVATE
    "$<$<CXX_COMPILER_ID:MSVC>:/O2>"
    "$<$<CXX_COMPILER_ID:MSVC>:/std:c++14>"
    "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O3>"
    "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-pipe>"
)

install(TARGETS _native DESTINATION nes_py)
