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

add_executable(sm_ptex_tool ${SRC} main.cpp)
target_link_libraries(sm_ptex_tool ${PTEX_LIBRARY}
				   ${ZLIB_LIBRARY})

install(TARGETS sm_ptex_tool DESTINATION ${INSTALL_BIN_PATH})