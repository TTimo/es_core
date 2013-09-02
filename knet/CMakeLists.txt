cmake_minimum_required( VERSION 2.8 )
cmake_policy( SET CMP0007 NEW ) # this one it actually complained

project( knet_test )

# FIXME: we have to look for boost and other kNet deps ourselves?
# instead, leverage the work that kNet already does? use kNetLinkLibraries?
find_package( Boost COMPONENTS system thread )
if (Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  link_directories(${Boost_LIBRARY_DIRS})
else()
  message(FATAL_ERROR "Boost not found!")
endif()

if ( UNIX )
  add_definitions( -DUNIX )
endif ( UNIX )

include_directories( "${CMAKE_SOURCE_DIR}/../../kNet/include" )
add_executable( knet_test knet_test.cpp )
target_link_libraries( knet_test "${CMAKE_SOURCE_DIR}/../../kNet/lib/libkNet.a" )
target_link_libraries( knet_test ${Boost_LIBRARIES} )
