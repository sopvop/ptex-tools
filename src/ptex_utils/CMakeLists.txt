include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

include_directories(SYSTEM ${PTEX_INCLUDE_DIR}
                           ${PYTHON_INCLUDE_DIRS})

set(SRC ptex_merge.cpp
        ptex_reverse.cpp
        ptex_info.cpp
        make_constant.cpp
	ptex_conform.cpp
        objreader.cpp
        mesh.cpp
        helpers.cpp)

include(GenerateExportHeader)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

add_library(ptexutils ${SRC})

generate_export_header(ptexutils)

target_link_libraries(
    ptexutils
  PRIVATE
    PTex::lib
    ${ZLIB_LIBRARY}
    Boost::filesystem
    Boost::system
)

if(BUILD_SHARED_LIBS)
  target_compile_definitions(
    ptexutils
  PUBLIC
    PTEXUTILS_SHARED
  PRIVATE
    PTEXUTILS_EXPORTS
)
endif()

set_target_properties(ptexutils PROPERTIES
  SOVERSION 1.1)

install(TARGETS ptexutils
  EXPORT PtexUtilsTargets
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}
  ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
  INCLUDES DESTINATION ${INCLUDE_INSTALL_DIR})

install(FILES ptexutils.hpp
    "${CMAKE_CURRENT_BINARY_DIR}/ptexutils_export.h"
  DESTINATION "${INCLUDE_INSTALL_DIR}"
  COMPONENT dev)

export(EXPORT PtexUtilsTargets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/PtexUtilsTargets.cmake")

install(EXPORT PtexUtilsTargets
  FILE "PtexUtilsTargets.cmake"
  DESTINATION ${LIB_INSTALL_DIR}/cmake/ptexutils
)

add_executable(ptex-tool ${SRC} main.cpp)
target_link_libraries(ptex-tool ${PTEX_LIBRARY}
                                ${ZLIB_LIBRARY}
                                Boost::filesystem
                                Boost::system)
install(TARGETS ptex-tool DESTINATION ${BIN_INSTALL_DIR})

set_target_properties(ptex-tool
  PROPERTIES INSTALL_RPATH "\$ORIGIN/../lib;"
)

if (BUILD_PYTHON_MODULE)

  PYTHON_ADD_MODULE(cptexutils ${SRC} py_ptx_utils.cpp)

  target_compile_definitions(cptexutils PRIVATE "-DPTEXUTILS_STATIC_DEFINE")

  target_link_libraries(cptexutils ${PTEX_LIBRARY}
                                   ${PYTHON_LIBRARIES}
                                   ${ZLIB_LIBRARIES}
                                   Boost::filesystem
                                   Boost::system)

  install(TARGETS cptexutils
    DESTINATION ${PYTHON_INSTALL_DIR}/ptexutils/)
endif()
