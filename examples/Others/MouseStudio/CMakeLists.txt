# FabGL2 example: MouseStudio
cmake_minimum_required(VERSION 3.22)

# path of FabGL repository (can be overridden by FABGL_PATH environment variable)
if(DEFINED ENV{FABGL_PATH})
  set(FABGL_PATH $ENV{FABGL_PATH})
else()
  set(FABGL_PATH "${CMAKE_CURRENT_LIST_DIR}/../../..")
endif()
set(EXTRA_COMPONENT_DIRS "${FABGL_PATH}/components/fabgl")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(MINIMAL_BUILD ON)
project(MouseStudio)
