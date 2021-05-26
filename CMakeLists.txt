# set minimum cmake version
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# project name and language
project(thames LANGUAGES C)

add_executable(thames
    thames/cmdline.c
    thames/errcheck.c
    thames/extensions.c
    thames/isis.c
    thames/isisfile.c
    thames/isisload.c
    thames/thames.c
    thames/utils.c
    thames/version.c
    thames/z80.c
    thames/cbops.h
    thames/cmdline.h
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
    thames/edops.h
    thames/errcheck.h
    thames/extensions.h
    thames/isis.h
    thames/isisfile.h
    thames/isisload.h
    thames/thames.h
    thames/z80.h
    thames/z80ops.h
    ${CMAKE_CURRENT_BINARY_DIR}/Generated/version.h
    )

target_compile_options(thames
    PRIVATE
    "-D_CRT_SECURE_NO_WARNINGS"
    )

target_include_directories(thames PUBLIC
	$<BUILD_INTERFACE: ${CMAKE_CURRENT_BINARY_DIR}/Generated>)

add_custom_target(
getversion
COMMAND perl ${thames_SOURCE_DIR}/Scripts/version.pl ${CMAKE_CURRENT_BINARY_DIR}/Generated/ ${CMAKE_CURRENT_BINARY_DIR}/Generated/version.h
BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/Generated/GIT_VERSION_INFO ${CMAKE_CURRENT_BINARY_DIR}/Generated/version.h
WORKING_DIRECTORY ${thames_SOURCE_DIR}/thames
)

add_dependencies(thames getversion)

include(CheckIncludeFiles)
CHECK_INCLUDE_FILES(sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILES(sys/stat.h HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILES(fcntl.h HAVE_FNCTL_H)
CHECK_INCLUDE_FILES(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILES(errno.h HAVE_ERRNO_H)
CHECK_INCLUDE_FILES(limits.h HAVE_LIMITS_H)
CHECK_INCLUDE_FILES(conio.h HAVE_CONIO_H)
CHECK_INCLUDE_FILES(io.h HAVE_IO_H)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
configure_file(config.in ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)


