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

#include "OgreRoot.h"

#include "../game_main.h"

#include "shared_render_state.h"
#include "game.h"

void parse_mouse_state( char * mouse_state, Ogre::Quaternion & orientation, uint8_t & buttons ) {
  char * start = mouse_state;
  char * end = strchr( start, ' ' );
  end[0] = '\0';
  orientation.w = (float)atof( start );
  start = end + 1;
  end = strchr( start, ' ' );
  end[0] = '\0';
  orientation.x = (float)atof( start );
  start = end + 1;
  end = strchr( start, ' ' );
  end[0] = '\0';
  orientation.y = (float)atof( start );
  start = end + 1;
  end = strchr( start, ' ' );
  end[0] = '\0';
  orientation.z = (float)atof( start );
  start = end + 1;
  buttons = atoi( start );
}

void parse_kb_state( char * kb_state, uint8_t & w, uint8_t & a, uint8_t & s, uint8_t & d, uint8_t & spc, uint8_t & alt ) {
  char * start = kb_state;
  char * end = strchr( start, ' ' );
  end[0] = '\0';
  w = atoi( start );
  start = end + 1;
  end = strchr( start, ' ' );
  end[0] = '\0';
  a = atoi( start );
  start = end + 1;
  end = strchr( start, ' ' );
  end[0] = '\0';
  s = atoi( start );
  start = end + 1;
  end = strchr( start, ' ' );
  end[0] = '\0';
  d = atoi( start );
  start = end + 1;
  end = strchr( start, ' ' );
  end[0] = '\0';
  spc = atoi( start );
  start = end + 1;
  alt = atoi( start );
}

void game_init( GameThreadSockets & gsockets, GameState & gs, SharedRenderState & srs ) {
  srs.position = Ogre::Vector3::ZERO;
}

void game_tick( GameThreadSockets & gsockets, GameState & gs, SharedRenderState & srs, unsigned int now ) {
  Ogre::Quaternion o;
  uint8_t b;
  zstr_send( gsockets.zmq_input_req, "mouse_state" );
  char * mouse_state = zstr_recv( gsockets.zmq_input_req );
  parse_mouse_state( mouse_state, o, b );
  free( mouse_state );

  uint8_t w, a, s, d, spc, alt;
  zstr_send( gsockets.zmq_input_req, "kb_state" );
  char * kb_state = zstr_recv( gsockets.zmq_input_req );
  parse_kb_state( kb_state, w, a, s, d, spc, alt );
  free( kb_state );

  // yaw 0 looks towards -Z
  float yaw = o.getYaw().valueRadians();
  Ogre::Vector3 xp( cosf( yaw ), 0.0, -sinf( yaw ) );
  Ogre::Vector3 yp( -sinf( yaw ), 0.0, -cosf( yaw ) );

  float speed = 10.0f;
  if ( w ) {
    srs.position += speed * yp;
  }
  if ( s ) {
    srs.position -= speed * yp;
  }
  if ( a ) {
    srs.position -= speed * xp;
  }
  if ( d ) {
    srs.position += speed * xp;
  }
  if ( spc ) {
    srs.position[1] += speed;
  }
  if ( alt ) {
    srs.position[1] -= speed;
  }
}

void emit_render_state( void * socket, unsigned int time, SharedRenderState & srs ) {
  zstr_sendf( socket, "%d %f %f %f", time, srs.position[0], srs.position[1], srs.position[2] );  
}
