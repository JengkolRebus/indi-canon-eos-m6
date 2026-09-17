cmake_minimum_required(VERSION 3.10)

project(indi_canon_eos_m6 VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_path(INDI_INCLUDE_DIR
    NAMES indiccd.h
    PATHS
        /usr/include/libindi
        /usr/include
    REQUIRED
)

find_library(INDI_DRIVER_LIBRARY
    NAMES indidriver
    PATHS
        /usr/lib/x86_64-linux-gnu
        /lib/x86_64-linux-gnu
    REQUIRED
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GPHOTO2 REQUIRED IMPORTED_TARGET libgphoto2)

add_executable(indi_canon_eos_m6
    indi_canon_eos_m6.cpp
)

target_include_directories(indi_canon_eos_m6 PRIVATE
    ${INDI_INCLUDE_DIR}
)

target_compile_options(indi_canon_eos_m6 PRIVATE
    -Wall -Wextra -Wpedantic
)

target_link_libraries(indi_canon_eos_m6 PRIVATE
    ${INDI_DRIVER_LIBRARY}
    PkgConfig::GPHOTO2
)

install(TARGETS indi_canon_eos_m6 RUNTIME DESTINATION bin)
