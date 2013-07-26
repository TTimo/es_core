# -*- Mode: python -*-

import os, platform, subprocess

( system, node, release, version, machine, processor ) = platform.uname()

env = Environment( ENV = os.environ )

if ( system == 'Darwin' ):
    env['SDL2_CONFIG'] = '/Users/timo/SDL/autocrap.install/bin/sdl2-config'
    env['OGRE_SRC'] = '/Users/timo/ogre'
    env['MACPORTS_PREFIX'] = '/opt/local'
else:
    env['SDL2_CONFIG'] = '/home/timo/SDL/install/bin/sdl2-config'
    env['OGRE_SRC'] = '/home/timo/ogre'

env.Append( CCFLAGS = [ '-g', ] )

env.ParseConfig( '%s --cflags --libs' % env['SDL2_CONFIG'] )

env.Append( CPPPATH = os.path.join( env['OGRE_SRC'], 'include' ) )
env.Append( CPPPATH = os.path.join( env['OGRE_SRC'], 'OgreMain/include' ) )
env.Append( CPPPATH = os.path.join( env['OGRE_SRC'], 'PlugIns/BSPSceneManager/include' ) )

source = [ 'main.cpp', 'render_main.cpp', 'game_main.cpp' ]

if ( system == 'Darwin' ):
    env.Append( CCFLAGS = [ '-arch', 'x86_64', ] )
    env.Append( CPPPATH = os.path.join( env['OGRE_SRC'], 'RenderSystems/GL/include' ) )
    env.Append( CPPPATH = os.path.join( env['OGRE_SRC'], 'RenderSystems/GL/include/OSX' ) )
    env.Append( CPPPATH = os.path.join( env['MACPORTS_PREFIX'], 'include' ) ) # boost, zmq etc.
    env.Append( FRAMEWORKPATH = os.path.join( env['OGRE_SRC'], 'lib/Release' ) )
    env.Append( FRAMEWORKS = [ 'Ogre', 'Foundation', 'AppKit' ] )
    env.Append( LIBS = [ 'czmq', 'zmq' ] )
    env.Append( LIBPATH = os.path.join( env['MACPORTS_PREFIX'], 'lib' ) )
    env.Append( LINKFLAGS = [ '-headerpad_max_install_names', ] )
    source.append( 'OSX_wrap.mm' )
else:
    env.Append( LIBS = 'OgreMain' )
    env.Append( RPATH = [ os.path.join( env['OGRE_SRC'], 'lib' ) ] )
    env.Append( LIBPATH = os.path.join( env['OGRE_SRC'], 'lib' ) )
    env.ParseConfig( 'pkg-config libzmq --cflags --libs' )
    env.ParseConfig( 'pkg-config libczmq --cflags --libs' )
    env.Append( RPATH = [ '/usr/local/lib' ] )
    env.Append( RPATH = [ '.' ] )

# see site_scons/site_init.py
env = AppendOSXBundleBuilder( env )

template_env = env.Clone()
template_env.Append( CPPPATH = [ 'template_src' ] )
template_env.VariantDir( 'build/template_src', '.' )
template = template_env.Program( 'template', [ os.path.join( 'build/template_src', s ) for s in source + [ 'template_src/game.cpp', 'template_src/render.cpp' ] ] )
template_bundle = template_env.Bundle( Dir( 'template.app' ), template )

scene_env = env.Clone()
scene_env.Append( CPPPATH = [ 'scene_load_src' ] )
scene_env.VariantDir( 'build/scene_load_src', '.' )
scene = scene_env.Program( 'scene', [ os.path.join( 'build/scene_load_src', s ) for s in source + [ 'scene_load_src/game.cpp', 'scene_load_src/render.cpp' ] ] )
scene_bundle = scene_env.Bundle( Dir( 'scene.app' ), scene )

head_env = env.Clone()
head_env.Append( CPPPATH = [ 'head_src' ] )
head_env.VariantDir( 'build/head_src', '.' )
head = head_env.Program( 'head', [ os.path.join( 'build/head_src', s ) for s in source + [ 'head_src/game.cpp', 'head_src/render.cpp' ] ] )
head_bundle = head_env.Bundle( Dir( 'head.app' ), head )

bsp_env = env.Clone()
bsp_env.Append( CPPPATH = [ 'bsp_src' ] )
bsp_env.VariantDir( 'build/bsp_src', '.' )
bsp = bsp_env.Program( 'bsp', [ os.path.join( 'build/bsp_src', s ) for s in source + [ 'bsp_src/game.cpp', 'bsp_src/render.cpp' ] ] )
bsp_bundle = bsp_env.Bundle( Dir( 'bsp.app' ), bsp )
