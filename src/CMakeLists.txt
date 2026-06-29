# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

file(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.cpp ${CMAKE_SOURCE_DIR}/src/*.c)
list(FILTER app_sources EXCLUDE REGEX "gui_old.cpp")

idf_component_register(SRCS ${app_sources})
