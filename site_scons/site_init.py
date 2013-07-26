# scons imports this module at startup. see http://www.scons.org/doc/production/HTML/scons-user/x3822.html

import os, platform
( system, node, release, version, machine, processor ) = platform.uname()

from SCons.Script import *

def _DoAssembleBundle( target, source, env ):
    topdir = os.getcwd()
    try:
        print('create the bundle structure for %s' % str( target[0] ) )
        bundle_dir = str( target[0] )
        source_bin = str( source[0] )
        macos_dir = os.path.join( bundle_dir, 'Contents/MacOS' )
        os.makedirs( macos_dir )
        os.chdir( macos_dir )
        os.symlink( os.path.join( '../../..', source_bin ), source_bin )
        os.chdir( topdir )
        resource_path = os.path.join( bundle_dir, 'Contents/Resources' )
        os.makedirs( resource_path )
        contents_dir = os.path.join( bundle_dir, 'Contents' )
        os.chdir( contents_dir )
        ogre_bin_dir = os.path.join( env['OGRE_SRC'], 'lib/Release' )
        os.symlink( ogre_bin_dir, 'Frameworks' )
        os.symlink( ogre_bin_dir, 'Plugins' )
        os.chdir( topdir )
    except:
        os.chdir( topdir )
        raise

def AppendOSXBundleBuilder( env ):
    if ( system == 'Darwin' ):
        b = Builder( action = _DoAssembleBundle )
    else:
        # dummy builder that does nothing
        b = Builder( action = '' )
    env.Append( BUILDERS = { 'Bundle' : b } )
    return env

