cmake_minimum_required(VERSION 2.8.10)
project(ptextools CXX)


set(ptexutils_VERSION 0.1.0)

set(PYTHON_INSTALL_DIR "python" CACHE PATH "python module install dir")
set(BIN_INSTALL_DIR "bin" CACHE PATH "executable install dir")
set(INCLUDE_INSTALL_DIR "include" CACHE PATH "Include install dir")
set(LIB_INSTALL_DIR "lib" CACHE PATH "Library install dir")

set(BUILD_PYTHON_MODULE ON CACHE BOOL "Should python module be built")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(PTex REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Boost COMPONENTS filesystem system REQUIRED)
if (BUILD_PYTHON_MODULE)
  find_package(PythonInterp REQUIRED)
  find_package(PythonLibs REQUIRED)

  if ( NOT PYTHON_VERSION_STRING VERSION_EQUAL  PYTHONLIBS_VERSION_STRING )
    message ( FATAL_ERROR "Version mismatch between python interpreter and libraries")
  endif()
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-deprecated -pthread")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--no-undefined -Wl,-z,defs -Wl,-O2 -std=c++11")
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
endif()

add_subdirectory(src/ptex_utils)

if (BUILD_PYTHON_MODULE)
  install(FILES ptexutils/__init__.py
    DESTINATION ${PYTHON_INSTALL_DIR}/ptexutils/)
endif()

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/PtexUtilsConfigVersion.cmake"
  VERSION ${ptexutils_VERSION}
  COMPATIBILITY SameMajorVersion
)

configure_file(PtexUtilsConfig.cmake
  "${CMAKE_CURRENT_BINARY_DIR}/src/ptex_utils/PtexUtilsConfig.cmake"
  COPYONLY
)

install(
  FILES
    PtexUtilsConfig.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/PtexUtilsConfigVersion.cmake"
  DESTINATION
    ${LIB_INSTALL_DIR}/cmake/ptexutils
  COMPONENT
    dev
)
