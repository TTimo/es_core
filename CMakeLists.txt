cmake_minimum_required( VERSION 2.8 )
cmake_policy( SET CMP0007 NEW ) # this one it actually complained

# NOTE: we assume that CMAKE_INSTALL_PREFIX is setup, and that all the dependencies are installed there already

project( es_core )

find_package( OGRE REQUIRED )

# FIXME: replace with a proper SDL2 cmake package, if/when there is such a thing
# or piggyback on the sdl2 pkg-config (PKG_CONFIG_PATH and all that)
# --cflags: -D_THREAD_SAFE -I/Users/timo/es_core/run/include/SDL2 
# --libs: -L/Users/timo/es_core/run/lib -lSDL2 
include_directories( "${CMAKE_INSTALL_PREFIX}/include/SDL2" )
link_directories( "${CMAKE_INSTALL_PREFIX}/lib" )

include_directories( ${OGRE_INCLUDE_DIRS} )

# NOTE: under the assumption that all the es_core dependencies are getting installed to the same prefix,
# the above has defined all the includes and library directories needed for ZeroMQ

# specific to BSP target

# FIXME: fix ogre cmake so it sets this up properly
#include_directories( ${OGRE_Plugin_BSPSceneManager_INCLUDE_DIR} )
#pkg_message( BSP "BSP: ${OGRE_Plugin_BSPSceneManager_INCLUDE_DIR}" )
include_directories( "${CMAKE_INSTALL_PREFIX}/include/OGRE/Plugins/BSPSceneManager" )

include_directories( "bsp_src" )
add_executable( bsp main.cpp game_main.cpp render_main.cpp bsp_src/game.cpp bsp_src/render.cpp )
target_link_libraries( bsp czmq zmq )
target_link_libraries( bsp SDL2 )
target_link_libraries( bsp ${OGRE_LIBRARIES} )

if ( APPLE )
  find_library( FOUNDATION_FRAMEWORK Foundation )
  find_library( APPKIT_FRAMEWORK AppKit )
  target_link_libraries( bsp ${FOUNDATION_FRAMEWORK} ${APPKIT_FRAMEWORK} )
endif ( APPLE )

# TODO: other targets
