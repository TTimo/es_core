/*
Copyright (c) 2013, Timothee Besset
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Timothee Besset nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL Timothee Besset BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "OgreRoot.h"
#include "OgrePlatform.h"
#include "OgreOSXCocoaWindow.h"

#include "SDL.h"
#include "SDL_opengl.h"
#include "SDL_sysvideo.h"

#include "OSX_wrap.h"

#include "SDL_cocoawindow.h"

Ogre::String OSX_cocoa_view( SDL_Window * window ) {
  SDL_WindowData * cocoa_window_data = (SDL_WindowData*)window->driverdata;
  NSView * view = [cocoa_window_data->nswindow contentView];
  return Ogre::StringConverter::toString( (unsigned long)view );
}

void OSX_GL_set_current( Ogre::RenderWindow * ogre_render_window ) {
  Ogre::OSXCocoaWindow * cocoa_window = (Ogre::OSXCocoaWindow*)ogre_render_window;
  NSOpenGLContext * context = cocoa_window->nsopenGLContext();
  [context makeCurrentContext];
}

void OSX_GL_clear_current( Ogre::RenderWindow * ogre_render_window ) {
  // NOTE: a static, doesn't need the param?
  // see /System/Library/Frameworks/AppKit.framework/Versions/C/Headers/NSOpenGL.h
  //  Ogre::OSXCocoaWindow * cocoa_window = (Ogre::OSXCocoaWindow*)ogre_render_window;
  //  NSOpenGLContext * context = cocoa_window->nsopenGLContext();
  [NSOpenGLContext clearCurrentContext]; 
}
