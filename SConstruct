# -*- Mode: python -*-

import os, platform, subprocess

( system, node, release, version, machine, processor ) = platform.uname()

if ( system == 'Darwin' ):
    SDL2_CONFIG = '/Users/timo/SDL/autocrap.install/bin/sdl2-config'
    OGRE_SRC = '/Users/timo/ogre'
else:
    SDL2_CONFIG = '/home/timo/SDL/install/bin/sdl2-config'
    OGRE_SRC = '/home/timo/ogre'

env = Environment( ENV = os.environ )

env.Append( CCFLAGS = [ '-g', ] ) # '-O4' ] )

env.ParseConfig( '%s --cflags --libs' % SDL2_CONFIG )

env.Append( CPPPATH = os.path.join( OGRE_SRC, 'include' ) )
env.Append( CPPPATH = os.path.join( OGRE_SRC, 'OgreMain/include' ) )

source = [ 'main.cpp', 'render_main.cpp', 'game_main.cpp' ]

if ( system == 'Darwin' ):
    env.Append( CCFLAGS = [ '-arch', 'x86_64', '-mmacosx-version-min=10.6' ] )
    env.Append( CPPPATH = os.path.join( OGRE_SRC, 'RenderSystems/GL/include' ) )
    env.Append( CPPPATH = os.path.join( OGRE_SRC, 'RenderSystems/GL/include/OSX' ) )
    env.Append( CPPPATH = '/opt/local/include' ) # boost, zmq etc.
    env.Append( FRAMEWORKPATH = '/Users/timo/ogre/lib/Release' )
    env.Append( FRAMEWORKS = [ 'Ogre', 'Foundation', 'AppKit' ] )
    env.Append( LIBS = [ 'czmq', 'zmq' ] )
    env.Append( LIBPATH = '/opt/local/lib' )
    env.Append( LINKFLAGS = [ '-headerpad_max_install_names', '-mmacosx-version-min=10.6' ] )
    source.append( 'OSX_wrap.mm' )
else:
    env.Append( LIBS = 'OgreMain' )
    env.Append( RPATH = [ os.path.join( OGRE_SRC, 'lib' ) ] )
    env.Append( LIBPATH = os.path.join( OGRE_SRC, 'lib' ) )
    env.ParseConfig( 'pkg-config libzmq --cflags --libs' )
    env.ParseConfig( 'pkg-config libczmq --cflags --libs' )
    env.Append( RPATH = [ '/usr/local/lib' ] )
    env.Append( RPATH = [ '.' ] )

# TODO: some refactoring needed to properly support multiple targets here..

core = env.Program( 'core', source + [ 'input_test/game.cpp', 'input_test/render.cpp' ] )

env.Append( CPPPATH = [ 'mesh_loader' ] )
mesh = env.Program( 'mesh', source + [ 'mesh_loader/game.cpp', 'mesh_loader/render.cpp' ] )

if ( system == 'Darwin' ):

    def bundle_up( target, source, env ):
        # point to the bundled libraries
        # see http://blog.onesadcookie.com/2008/01/installname-magic.html
        # this is specific to my setup atm, the logic could be made more generic though
        def check_call( cmd ):
            print( cmd )
            subprocess.check_call( cmd, shell = True )
        map( check_call, [
            'cp "%s" "%s"' % ( source[0], target[0] ),
            './binaries/dylibbundler -od -b -d binaries/core.app/Contents/libs -x binaries/core.app/Contents/MacOS/core',
            ] )

    # NOTE: not in the default target list, invoke 'scons binaries/core.app/Contents/MacOS/core' to update it
    bundle_core = env.Command( 'binaries/core.app/Contents/MacOS/core', core, Action( bundle_up ) )
