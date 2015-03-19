include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(SYSTEM ${PTEX_INCLUDE_DIR}
                           ${PYTHON_INCLUDE_DIRS})
set(SRC ptex_merge.cpp
        ptex_reverse.cpp
        make_constant.cpp
        objreader.cpp
        helpers.cpp)

PYTHON_ADD_MODULE(cptexutils ${SRC} py_ptx_utils.cpp)
target_link_libraries(cptexutils ${PTEX_LIBRARY}
                                 ${PYTHON_LIBRARIES}
                                 ${ZLIB_LIBRARIES})

install(TARGETS cptexutils
  DESTINATION ${PYTHON_INSTALL_DIR}/ptexutils/)

add_executable(ptex-tool ${SRC} main.cpp)
target_link_libraries(ptex-tool ${PTEX_LIBRARY}
                                ${ZLIB_LIBRARY})

install(TARGETS ptex-tool DESTINATION ${BIN_INSTALL_DIR})
