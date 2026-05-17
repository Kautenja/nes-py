cmake_minimum_required(VERSION 3.21...4.2)

if(DEFINED SKBUILD_PROJECT_NAME)
    set(NES_EMU_DEFAULT_BUILD_PYTHON_EXTENSION ON)
else()
    set(NES_EMU_DEFAULT_BUILD_PYTHON_EXTENSION OFF)
endif()

if(NOT DEFINED SKBUILD_PROJECT_NAME)
    set(SKBUILD_PROJECT_NAME nes_py)
endif()
if(NOT DEFINED SKBUILD_PROJECT_VERSION)
    set(SKBUILD_PROJECT_VERSION 0.0.0)
endif()

project(${SKBUILD_PROJECT_NAME} VERSION ${SKBUILD_PROJECT_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(
    NES_EMU_BUILD_PYTHON_EXTENSION
    "Build the nes_py._native Python extension"
    ${NES_EMU_DEFAULT_BUILD_PYTHON_EXTENSION}
)
option(NES_EMU_BUILD_TESTS "Build native nes_emu Catch2 tests" OFF)
option(NES_EMU_BUILD_BENCHMARKS "Build native nes_emu Catch2 benchmarks" OFF)

set(NES_PY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/nes_py")
set(NES_NATIVE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/nes_emu")

file(GLOB_RECURSE NES_NATIVE_SOURCES CONFIGURE_DEPENDS
    "${NES_NATIVE_ROOT}/src/nes_emu/*.cpp"
)

function(configure_nes_emu_native_target target_name)
    target_include_directories(${target_name} PRIVATE
        "${NES_NATIVE_ROOT}/include"
    )
    target_compile_options(${target_name} PRIVATE
        "$<$<CXX_COMPILER_ID:MSVC>:/std:c++14>"
        "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-pipe>"
    )
endfunction()

if(NES_EMU_BUILD_PYTHON_EXTENSION)
    find_package(Python COMPONENTS Interpreter Development.Module NumPy REQUIRED)

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

    Python_add_library(_native MODULE WITH_SOABI
        "${NES_GENERATED_SOURCE}"
        ${NES_NATIVE_SOURCES}
    )

    if(DEFINED SKBUILD AND SKBUILD_STATE STREQUAL "editable" AND
            CMAKE_CURRENT_BINARY_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        set_target_properties(_native PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${NES_PY_ROOT}$<0:>"
            RUNTIME_OUTPUT_DIRECTORY "${NES_PY_ROOT}$<0:>"
        )
    endif()

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
endif()

if(NES_EMU_BUILD_TESTS OR NES_EMU_BUILD_BENCHMARKS)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.4
    )
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
endif()

if(NES_EMU_BUILD_TESTS)
    enable_testing()
    file(GLOB_RECURSE NES_EMU_TEST_SOURCES CONFIGURE_DEPENDS
        "${NES_NATIVE_ROOT}/test/nes_emu/*.cpp"
    )
    add_executable(nes_emu_tests
        ${NES_NATIVE_SOURCES}
        ${NES_EMU_TEST_SOURCES}
    )
    configure_nes_emu_native_target(nes_emu_tests)
    target_include_directories(nes_emu_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${NES_NATIVE_ROOT}/test"
    )
    target_link_libraries(nes_emu_tests PRIVATE Catch2::Catch2WithMain)
    include(Catch)
    catch_discover_tests(nes_emu_tests)
endif()

if(NES_EMU_BUILD_BENCHMARKS)
    file(GLOB_RECURSE NES_EMU_BENCHMARK_SOURCES CONFIGURE_DEPENDS
        "${NES_NATIVE_ROOT}/benchmark/nes_emu/*.cpp"
    )
    add_executable(nes_emu_benchmarks
        ${NES_NATIVE_SOURCES}
        ${NES_EMU_BENCHMARK_SOURCES}
    )
    configure_nes_emu_native_target(nes_emu_benchmarks)
    target_include_directories(nes_emu_benchmarks PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${NES_NATIVE_ROOT}/test"
    )
    target_link_libraries(nes_emu_benchmarks PRIVATE Catch2::Catch2WithMain)
endif()
