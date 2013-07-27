Binaries for es_core demo applications
======================================

The latest binaries can be downloaded from https://s3.amazonaws.com/es_core/es_core.binaries.zip [1]

head.exe
--------

This is the original demo that was provided with es_core. A simple head bouncing in front of the camera, which can be spun by mouse drags. The code shows how input can be polled both from the game logic and the render thread, and how state is split between game state, and shared render state for interpolation.

scene.exe
---------

If you run without arguments, scene.exe will load media/models/uptown.mesh and offer a basic fly through. If you pass an argument, for instance 'scene.mesh', it will load media/models/scene.mesh.

The rendering is a flat shaded material, this is meant to facilitate examining geometry.

bsp.exe
-------

This will load an idTech3 .bsp file. It can look in zip-format .pk3 files and load textures as well. You need to pass two parameters on the command line, the location of the pk3 files and the name of the bsp file.

Example:
bsp.exe C:\Quake3\baseq3 maps/q3dm13.bsp

template.exe
------------

This is an empty app that can be used to start a new project easily. I keep it around to have a clean view of what the minimal es_core framework API is.

Platforms
=========

Windows
-------

Make sure you have Microsoft Visual Studio 2012 redistributable installed. If you don't (know), run the provided vcredist_x86.exe [2]

GNU/Linux and MacOS X
---------------------

You will have to compile yourself for those. I do most of my development on those platforms, so it works great, but packaging compatible binaries for these platforms is tricky and I haven't invested the time into doing this properly yet.

Bundled assets
==============

media/ is a smaller version of Ogre 1.8's Samples/Media/, copied here for easier distribution of binaries.
For more information, see https://bitbucket.org/sinbad/ogre/src/cf06d4ed07933c23985037182937139f56f15015/Samples/Media/?at=v1-8

media/models/uptown.mesh is a conversion of Urban Terror's map 'uptown' to Ogre's mesh format, distributed with permission from FrozenSand.
Authors may not use this level as a base to build additional levels.
For more information, see http://www.urbanterror.info/

References
==========

- [1] es_core binaries - https://s3.amazonaws.com/es_core/es_core.binaries.zip
- [2] Microsoft Visual Studio redistributable - http://www.microsoft.com/en-us/download/details.aspx?id=30679
