cmake_minimum_required( VERSION 2.8 )
cmake_policy( SET CMP0007 NEW ) # this one it actually complained

project( enet_test )

include_directories( "${CMAKE_INSTALL_PREFIX}/include" )
link_directories( "${CMAKE_INSTALL_PREFIX}/lib" )
add_executable( enet_test enet_test.cpp )
target_link_libraries( enet_test enet )
