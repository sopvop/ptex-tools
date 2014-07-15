cmake_minimum_required(VERSION 2.8.10)
project(ptextools)

set(INSTALL_PYTHON_PATH "python" CACHE PATH "Install python plugin here")
set(INSTALL_LIB_PATH "lib" CACHE PATH "Install library here")
set(INSTALL_BIN_PATH "bin" CACHE PATH "Install executables here")

find_package(PTex REQUIRED)
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED python)
find_package(ZLIB REQUIRED)

find_package(PythonInterp REQUIRED)
find_package(PythonLibs REQUIRED)
if ( NOT PYTHON_VERSION_STRING VERSION_EQUAL  PYTHONLIBS_VERSION_STRING )
  message ( FATAL_ERROR "Version mismatch between python interpreter and libraries")
endif()

if(CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-deprecated -pthread")
  set(LINK_FLAGS "-pthread -Wl,--no-undefined -Wl,-O1")
endif()

add_subdirectory(src/ptex_utils)

install(FILES ptexutils/__init__.py
  DESTINATION ${INSTALL_PYTHON_PATH}/ptexutils/)