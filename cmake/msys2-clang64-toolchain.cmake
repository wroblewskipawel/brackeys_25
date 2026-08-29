# Portable MSYS2 CLANG64 toolchain: Locates the toolchain and fails with setup guidance if missing

if(DEFINED ENV{MSYS2_ROOT})
    set(_msys2_root "$ENV{MSYS2_ROOT}")
else()
    set(_msys2_root "C:/msys64")
endif()

set(_msys2_clang_cxx "${_msys2_root}/clang64/bin/clang++.exe")

if(NOT EXISTS "${_msys2_clang_cxx}")
    message(FATAL_ERROR
    "MSYS2 CLANG64 was not found at '${_msys2_clang_cxx}'.\n"
    "Install MSYS2 from https://www.msys2.org/, then from an MSYS2 CLANG64 shell "
    "install the required toolchain using ./scripts/msys2-clang64-setup.sh.\n"
    "If MSYS2 is installed at a different location, set the MSYS2_ROOT environment "
    "variable to point at it and re-run CMake configure."
    )
endif()

set(CMAKE_C_COMPILER "${_msys2_root}/clang64/bin/clang.exe" CACHE FILEPATH "" FORCE)
set(CMAKE_CXX_COMPILER "${_msys2_clang_cxx}" CACHE FILEPATH "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS "-static" CACHE STRING "" FORCE)

set(ENV{PATH} "${_msys2_root}/clang64/bin;$ENV{PATH}")
