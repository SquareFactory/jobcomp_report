cmake_minimum_required(VERSION 3.9)

# Availables options
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_INSTALL_PREFIX /usr/lib/slurm)
set(SLURM_SRC_DIR ${CMAKE_SOURCE_DIR}/externals/slurm)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(THREADS_PREFER_PTHREAD_FLAG ON)

project(
  jobcomp_report
  VERSION 1.0.0
  DESCRIPTION "Job completion reporting plugin")

find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

add_compile_options(
  -O3
  -m64
  -std=c99
  -Wall
  -Wextra
  -Werror
  -Wshadow
  -Wformat=2
  -Wpointer-arith
  -Winit-self
  -Wcast-align
  -Wcast-qual
  -Wunreachable-code
  -fno-common
  -Wno-unused-parameter)

file(GLOB_RECURSE sources src/*.c src/*.h)

add_custom_command(
  OUTPUT ${SLURM_SRC_DIR}/slurm/slurm_version.h
  COMMAND ${SLURM_SRC_DIR}/configure
  WORKING_DIRECTORY ${SLURM_SRC_DIR}
  COMMENT "Configure slurm"
  VERBATIM)

add_custom_target(slurm_configure
                  DEPENDS ${SLURM_SRC_DIR}/slurm/slurm_version.h)

add_library(jobcomp_report SHARED ${sources})
add_library(jobcomp_report_static STATIC ${sources})

add_dependencies(jobcomp_report slurm_configure)
add_dependencies(jobcomp_report_static slurm_configure)

set_target_properties(jobcomp_report PROPERTIES C_STANDARD 11 PREFIX "")
set_target_properties(
  jobcomp_report_static
  PROPERTIES C_STANDARD 11
             PREFIX ""
             OUTPUT_NAME ${CMAKE_PROJECT_NAME})

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${SLURM_SRC_DIR})
target_include_directories(${CMAKE_PROJECT_NAME}_static PUBLIC ${SLURM_SRC_DIR})

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Threads::Threads curl m)

install(
  TARGETS jobcomp_report jobcomp_report_static
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX})
