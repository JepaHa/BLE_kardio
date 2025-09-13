cmake_minimum_required(VERSION 3.27)

# Helper function to find Zephyr base from west top directory
function(zephyr_base_from_west_topdir)
  execute_process(COMMAND west topdir OUTPUT_VARIABLE TOPDIR)
  string(STRIP ${TOPDIR} TOPDIR)
  execute_process(COMMAND west config zephyr.base OUTPUT_VARIABLE BASE)
  string(STRIP ${BASE} BASE)
  set(ENV{ZEPHYR_BASE} ${TOPDIR}/${BASE})
endfunction()

if(NOT DEFINED ENV{ZEPHYR_BASE})
  zephyr_base_from_west_topdir()
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zephyr-demo)

FILE(GLOB_RECURSE SOURCES src/*.c)
target_include_directories(app PRIVATE include)
target_sources(app PRIVATE ${SOURCES})
