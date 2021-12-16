cmake_minimum_required(VERSION 3.9)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_INSTALL_PREFIX /usr/lib)
set(SLURM_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/externals/slurm)
set(SLURM_SRC_DIR ${CMAKE_SOURCE_DIR}/externals/slurm)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_STATIC_LIBRARY_PREFIX "")

project(
  jobcomp_report
  VERSION 1.0.0
  DESCRIPTION "Job completion reporting plugin")

add_compile_options(-O3 -Wall -Wextra -Wno-unused-parameter)

add_library(jobcomp_report SHARED ${CMAKE_SOURCE_DIR}/src/jobcomp_report.c)
add_library(jobcomp_report_static STATIC
            ${CMAKE_SOURCE_DIR}/src/jobcomp_report.c)

set_target_properties(jobcomp_report PROPERTIES C_STANDARD 11 PREFIX "")
set_target_properties(
  jobcomp_report_static
  PROPERTIES C_STANDARD 11
             PREFIX ""
             OUTPUT_NAME jobcomp_report)

target_include_directories(jobcomp_report PUBLIC ${SLURM_INCLUDE_DIR}
                                                 ${SLURM_SRC_DIR})
target_include_directories(jobcomp_report_static PUBLIC ${SLURM_INCLUDE_DIR}
                                                        ${SLURM_SRC_DIR})
