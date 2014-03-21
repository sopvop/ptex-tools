include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(SYSTEM ${PTEX_INCLUDE_DIRS}
                           ${PYTHON_INCLUDE_DIRS}
                           ${Boost_INCLUDE_DIRS})

PYTHON_ADD_MODULE(cptexutils ptex_merge.cpp py_ptx_utils.cpp)
target_link_libraries(cptexutils ${PTEX_LIBRARY}
                                 ${Boost_LIBRARIES}
				 ${PYTHON_LIBRARIES}
				 ${ZLIB_LIBRARIES})

install(TARGETS cptexutils
  DESTINATION ${INSTALL_PYTHON_PATH}/ptexutils/)

add_executable(sm_ptex_merge ptex_merge.cpp ptex_merge_main.cpp)
target_link_libraries(sm_ptex_merge ${PTEX_LIBRARY}
				    ${ZLIB_LIBRARY})

install(TARGETS sm_ptex_merge DESTINATION ${INSTALL_BIN_PATH})