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

# BSP -----------------

add_executable( bsp main.cpp game_main.cpp render_main.cpp bsp_src/game.cpp bsp_src/render.cpp )
get_target_property( BSP_INCLUDES bsp INCLUDE_DIRECTORIES )
# FIXME: fix ogre cmake so it sets this up properly instead of digging it up ourselves
#pkg_message( BSP "BSP: ${OGRE_Plugin_BSPSceneManager_INCLUDE_DIR}" )
list( APPEND BSP_INCLUDES "${CMAKE_INSTALL_PREFIX}/include/OGRE/Plugins/BSPSceneManager" )
list( APPEND BSP_INCLUDES "${CMAKE_SOURCE_DIR}/bsp_src" )
set_property( TARGET bsp PROPERTY INCLUDE_DIRECTORIES ${BSP_INCLUDES} )
target_link_libraries( bsp czmq zmq )
target_link_libraries( bsp SDL2 )
target_link_libraries( bsp ${OGRE_LIBRARIES} )

# FIXME: APPLE
if ( APPLE )
  find_library( FOUNDATION_FRAMEWORK Foundation )
  find_library( APPKIT_FRAMEWORK AppKit )
  target_link_libraries( bsp ${FOUNDATION_FRAMEWORK} ${APPKIT_FRAMEWORK} )
endif ( APPLE )

# HEAD ----------------

add_executable( head main.cpp game_main.cpp render_main.cpp head_src/game.cpp head_src/render.cpp )
get_target_property( HEAD_INCLUDES head INCLUDE_DIRECTORIES )
list( APPEND HEAD_INCLUDES "${CMAKE_SOURCE_DIR}/head_src" )
set_property( TARGET head PROPERTY INCLUDE_DIRECTORIES ${HEAD_INCLUDES} )
target_link_libraries( head czmq zmq )
target_link_libraries( head SDL2 )
target_link_libraries( head ${OGRE_LIBRARIES} )

# SCENE ---------------

add_executable( scene main.cpp game_main.cpp render_main.cpp scene_load_src/game.cpp scene_load_src/render.cpp )
get_target_property( SCENE_INCLUDES scene INCLUDE_DIRECTORIES )
list( APPEND SCENE_INCLUDES "${CMAKE_SOURCE_DIR}/scene_load_src" )
set_property( TARGET scene PROPERTY INCLUDE_DIRECTORIES ${SCENE_INCLUDES} )
target_link_libraries( scene czmq zmq )
target_link_libraries( scene SDL2 )
target_link_libraries( scene ${OGRE_LIBRARIES} )

# TEMPLATE ------------

add_executable( template main.cpp game_main.cpp render_main.cpp template_src/game.cpp template_src/render.cpp )
get_target_property( TEMPLATE_INCLUDES template INCLUDE_DIRECTORIES )
list( APPEND TEMPLATE_INCLUDES "${CMAKE_SOURCE_DIR}/template_src" )
set_property( TARGET template PROPERTY INCLUDE_DIRECTORIES ${TEMPLATE_INCLUDES} )
target_link_libraries( template czmq zmq )
target_link_libraries( template SDL2 )
target_link_libraries( template ${OGRE_LIBRARIES} )
