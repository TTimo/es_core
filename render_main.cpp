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

#include "czmq.h"

#include "SDL.h"
#include "SDL_thread.h"
#include "SDL_syswm.h"

#include "OgreRoot.h"

#ifdef __APPLE__
#include <sys/stat.h>
#endif

#include "OSX_wrap.h"

// includes common to all projects
#include "render_main.h"

// per project includes:
// those headers are in each project subdirectory
#include "shared_render_state.h"
#include "render.h"

int render_thread( void * _parms ) {
  RenderThreadParms * parms = (RenderThreadParms*)_parms;
  RenderThreadSockets rsockets;

  void * zmq_control_socket = zsocket_new( parms->zmq_context, ZMQ_PAIR );
  {
    int ret = zsocket_connect( zmq_control_socket, "inproc://control_render" );
    assert( ret == 0 );
  }

  void * zmq_game_socket = zsocket_new( parms->zmq_context, ZMQ_PAIR );
  {
    int ret = zsocket_connect( zmq_game_socket, "inproc://game_render" );
    // NOTE: since both render thread and game thread get spun at the same time,
    // and the connect needs to happen after the bind,
    // it's possible this would fail on occasion? just loop a few times and retry?
    assert( ret == 0 );
  }

  rsockets.zmq_input_req = zsocket_new( parms->zmq_context, ZMQ_REQ );
  {
    int ret = zsocket_connect( rsockets.zmq_input_req, "inproc://input" );
    assert( ret == 0 );
  }

#ifdef __APPLE__
  OSX_GL_set_current( parms->ogre_window );
#else
  // NOTE: no SDL_GL_ on OSX when Ogre is in charge of the context!
  SDL_GL_MakeCurrent( parms->window, parms->gl_context );
#endif

  // internal render state, not part of the interpolation
  RenderState rs;

  // always interpolating between two states  
  SharedRenderState srs[2];
  unsigned int srs_index = 0;
  srs[0].game_time = srs[1].game_time = 0;

  render_init( parms, rs, srs[0] );

  while ( true ) {
    char * cmd = zstr_recv_nowait( zmq_control_socket );
    if ( cmd != NULL ) {
      assert( strcmp( cmd, "stop" ) == 0 );
      free( cmd );
      break; // exit the thread
    }
    while ( true ) {
      // any message from the game thread?
      char * game_tick = zstr_recv_nowait( zmq_game_socket );
      if ( game_tick == NULL ) {
	break;
      }

      srs_index ^= 1;
      parse_render_state( rs, srs[ srs_index ], game_tick );

      free( game_tick );
    }

    // skip rendering until enough data has come in to support interpolation
    if ( srs[0].game_time == srs[1].game_time ) { // 0 == 0
      continue;
    }
    unsigned int pre_render_time = SDL_GetTicks();
    float ratio = (float)( pre_render_time - srs[ srs_index ^ 1 ].game_time ) / (float)( srs[ srs_index ].game_time - srs[ srs_index ^ 1 ].game_time );
    printf( "render ratio %f\n", ratio );

    interpolate_and_render( rsockets, rs, ratio, srs[ srs_index ^ 1 ], srs[ srs_index ] );

    parms->root->_fireFrameStarted();
    parms->root->renderOneFrame();
    parms->root->_fireFrameRenderingQueued();
    if ( parms->gl_context != NULL ) {
      // glcontext != NULL <=> SDL initialized the GL context and manages the buffer swaps
      SDL_GL_SwapWindow( parms->window );
    }
    parms->root->_fireFrameEnded();
    // 'render latency': how late is the image we displayed
    // if vsync is off, it's the time it took to render the frame
    // if vsync is on, it's render time + time waiting for the buffer swap
    // NOTE: could display it terms of ratio also?
    unsigned int post_render_time = SDL_GetTicks();
    printf( "render latency %d ms\n", post_render_time - pre_render_time );
  }
  return 0;
}
