cmake_minimum_required(VERSION 3.29)

project(
  HealthNotes
  VERSION 1.0
  LANGUAGES CXX
)

set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)

add_library(minhook STATIC IMPORTED)
target_include_directories(minhook INTERFACE deps/minhook)
set_property(TARGET minhook PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/minhook/libMinHook.x64.lib)

add_library(loader STATIC IMPORTED)
target_include_directories(loader INTERFACE deps/loader)
set_property(TARGET loader PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/loader/loader.lib)

add_library(HealthNotes SHARED src/dllmain.cpp)
target_link_libraries(HealthNotes PRIVATE minhook loader)
target_include_directories(HealthNotes PRIVATE deps/json deps/loader deps/minhook)
target_compile_features(HealthNotes PRIVATE cxx_std_20)
