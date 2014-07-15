include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(SYSTEM ${PTEX_INCLUDE_DIRS}
                           ${PYTHON_INCLUDE_DIRS}
                           ${Boost_INCLUDE_DIRS})

set(SRC ptex_merge.cpp ptex_reverse.cpp)

PYTHON_ADD_MODULE(cptexutils ${SRC} py_ptx_utils.cpp)
target_link_libraries(cptexutils ${PTEX_LIBRARY}
                                 ${Boost_LIBRARIES}
				 ${PYTHON_LIBRARIES}
				 ${ZLIB_LIBRARIES})

install(TARGETS cptexutils
  DESTINATION ${INSTALL_PYTHON_PATH}/ptexutils/)

add_executable(ptex-tool ${SRC} main.cpp)
target_link_libraries(ptex-tool ${PTEX_LIBRARY}
				   ${ZLIB_LIBRARY})

install(TARGETS ptex-tool DESTINATION ${INSTALL_BIN_PATH})