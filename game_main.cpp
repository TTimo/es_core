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

#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "czmq.h"

#include "SDL.h"
#include "SDL_thread.h"

#include "OgreVector2.h"
#include "OgreVector3.h"

// includes common to all projects
#include "game_main.h"

// per project includes:
// those headers are in each project subdirectory
#include "shared_render_state.h"
#include "game.h"

int game_thread( void * _parms ) {
  GameThreadParms * parms = (GameThreadParms*)_parms;
  GameThreadSockets gsockets;

  GameState gs;
  SharedRenderState srs;

  gsockets.zmq_control_socket = zsocket_new( parms->zmq_context, ZMQ_PAIR );
  {
    int ret = zsocket_connect( gsockets.zmq_control_socket, "inproc://control_game" );
    assert( ret == 0 );
  }
  
  gsockets.zmq_render_socket = zsocket_new( parms->zmq_context, ZMQ_PAIR );
  zsocket_bind( gsockets.zmq_render_socket, "inproc://game_render" );

  gsockets.zmq_input_req = zsocket_new( parms->zmq_context, ZMQ_REQ );
  {
    int ret = zsocket_connect( gsockets.zmq_input_req, "inproc://input" );
    assert( ret == 0 );
  }

  game_init( gsockets, gs, srs );

  unsigned int baseline = SDL_GetTicks();
  unsigned int framenum = 0;
  while ( true ) {
    unsigned int now = SDL_GetTicks();
    unsigned int target_frame = ( now - baseline ) / GAME_DELAY;
    if ( framenum <= target_frame ) {
      framenum++;
      // NOTE: build the state of the world at t = framenum * GAME_DELAY,
      // under normal conditions that's a time in the future
      // (the exception to that is if we are catching up on ticking game frames)
      game_tick( gsockets, gs, srs, now );
      // notify the render thread that a new game state is ready.
      // on the next render frame, it will start interpolating between the previous state and this new one
      emit_render_state( gsockets.zmq_render_socket, baseline + framenum * GAME_DELAY, srs );
    } else {
      int ahead = framenum * GAME_DELAY - ( now - baseline );
      assert( ahead > 0 );
      printf( "game sleep %d ms\n", ahead );
      SDL_Delay( ahead );
    }
    char * cmd = zstr_recv_nowait( gsockets.zmq_control_socket );
    if ( cmd != NULL ) {
      assert( strcmp( cmd, "stop" ) == 0 );
      free( cmd );
      break;
    }
  }
  return 0;
}
