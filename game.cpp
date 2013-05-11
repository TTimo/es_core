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

#include "SDL.h"
#include "SDL_thread.h"

#include "OgreVector2.h"
#include "OgreVector3.h"

#include "czmq.h"

#include "game.h"
#include "game_render.h"

const int GAME_DELAY = 16;
const float GAME_TICK_FLOAT = (float)GAME_DELAY / 1000.0f;

const int ORIENTATION_LOG = 10;

typedef struct OrientationHistory_s {
  Uint32 t;
  Ogre::Quaternion o;
} OrientationHistory;

// only visible to the game logic
typedef struct GameState_s {
  void * zmq_control_socket;
  void * zmq_input_req;
  void * zmq_render_socket;

  float bounce;			// limits of the bounce area
  float speed;			// picked a speed to bounce around at startup
  bool mouse_pressed;		// go from mouse is pressed to click each time to change the control scheme
  Ogre::Vector2 direction;	// direction the head is moving on the plance
  Ogre::Vector3 rotation;	// rotation axis of the head
  Ogre::Degree rotation_speed;	// rotation speed of the head

  // use the last few frames of mouse input to build a smoothed angular velocity
  unsigned int orientation_index;
  OrientationHistory orientation_history[ ORIENTATION_LOG ];
  Ogre::Vector3 smoothed_angular;
  Ogre::Degree smoothed_angular_velocity;
} GameState;

void parse_mouse_state( char * mouse_state, Ogre::Quaternion & orientation, Uint8 & buttons ) {
  Ogre::Vector3 rotation_vector; // skipped over
  char * start = mouse_state;
  char * end = strchr( start, ' ' );
  end[0] = '\0';
  orientation.w = atof( start );
  start = end + 1;
  end = strchr( start, ' ' );
  end[0] = '\0';
  orientation.x = atof( start );
  start = end + 1;
  end = strchr( start, ' ' );
  end[0] = '\0';
  orientation.y = atof( start );
  start = end + 1;
  end = strchr( start, ' ' );
  end[0] = '\0';
  orientation.z = atof( start );
  start = end + 1;
  buttons = atoi( start );
}

void game_init( GameState & gs, SharedRenderState & rs ) {
  gs.zmq_control_socket = NULL;
  gs.zmq_input_req = NULL;
  gs.zmq_render_socket = NULL;
  time_t now;
  time( &now );
  srandom( now ); 
  gs.bounce = 25.0f;
  gs.speed = rand() % 60 + 40;
  float angle = (float)( rand() % 360 ) * 2.0f * M_PI / 360.0f;
  gs.direction = Ogre::Vector2( cosf( angle ), sinf( angle ) );
  rs.orientation = Ogre::Quaternion( Ogre::Radian( 0.0f ), Ogre::Vector3::UNIT_Z );
  rs.position = Ogre::Vector3::ZERO;
  gs.mouse_pressed = false;
  gs.rotation = Ogre::Vector3::UNIT_X;
  gs.rotation_speed = 0.0f;
  gs.orientation_index = 0;
  memset( gs.orientation_history, 0, sizeof( gs.orientation_history ) );
}

void game_tick( unsigned int now, GameState & gs, SharedRenderState & rs ) {
  // get the latest mouse buttons state and orientation
  zstr_send( gs.zmq_input_req, "mouse_state" );
  char * mouse_state = zstr_recv( gs.zmq_input_req );
  Uint8 buttons;
  Ogre::Quaternion orientation;
  parse_mouse_state( mouse_state, orientation, buttons );
  free( mouse_state );

  // at 16 ms tick and the last 10 orientations buffered, that's 150ms worth of orientation history
  gs.orientation_history[ gs.orientation_index ].t = now;
  gs.orientation_history[ gs.orientation_index ].o = orientation;
  gs.orientation_index = ( gs.orientation_index + 1 ) % ORIENTATION_LOG;

  // oldest orientation
  unsigned int q1_index = gs.orientation_index;
  // NOTE: the problem with using the successive orientations to infer an angular speed,
  // is that if the orientation is changing fast enough, this code will 'flip' the speed around
  // e.g. this doesn't work, need to use the XY mouse data to track angular speed
  // NOTE: uncomment the following line to use the full history, notice the 'flip' happens at much lower speed
  q1_index = ( q1_index + ORIENTATION_LOG - 2 ) % ORIENTATION_LOG; 
  Ogre::Quaternion q1 = gs.orientation_history[ q1_index ].o;
  Uint32 q1_t = gs.orientation_history[ q1_index ].t;
  Ogre::Quaternion omega = 2.0f * ( orientation - q1 ) * q1.UnitInverse() * ( 1000.0f / (float)( now - q1_t ) );
  omega.ToAngleAxis( gs.smoothed_angular_velocity, gs.smoothed_angular );
  //  printf( "%f %f %f - %f\n", gs.smoothed_angular.x, gs.smoothed_angular.y, gs.smoothed_angular.z, gs.smoothed_angular_velocity.valueDegrees() );
  rs.smoothed_angular = gs.smoothed_angular;

  if ( ( buttons & SDL_BUTTON( 1 ) ) != 0 ) {
    if ( !gs.mouse_pressed ) {
      gs.mouse_pressed = true;
      // changing the control scheme: the player is now driving the orientation of the head directly with the mouse
      // tell the input logic to reset the orientation to match the current orientation of the head
      zstr_sendf( gs.zmq_input_req, "mouse_reset %f %f %f %f", rs.orientation.w, rs.orientation.x, rs.orientation.y, rs.orientation.z );
      zstr_recv( gs.zmq_input_req ); // wait for ack from input
      // IF RENDER TICK HAPPENS HERE: render will not know that it should grab the orientation directly from the mouse,
      // but the orientation coming from game should still be ok?
      zstr_sendf( gs.zmq_render_socket, "# %s", "1" );
      // IF RENDER TICK HAPPENS HERE (before a new gamestate):
      // the now reset input orientation will combine with the old game state, that's bad
    }
  } else {
    if ( gs.mouse_pressed ) {
      gs.mouse_pressed = false;
      // changing the control scheme: the head will free spin and slow down for a bit, then it will resume bouncing around
      // the player looses mouse control, the game grabs latest orientation and angular velocity
      // the input thread was authoritative on orientation until now, so accept that as our starting orientation
      rs.orientation = orientation;
      gs.rotation_speed = gs.smoothed_angular_velocity;
      gs.rotation = gs.smoothed_angular;
      zstr_sendf( gs.zmq_render_socket, "# %s", "0" );
      // IF RENDER TICK HAPPENS HERE (before a new gamestate): render will pull the head orientation from the game state rather than input, but game state won't have the fixed orientation yet
    }
  }

  if ( rs.position.x > gs.bounce || rs.position.x < -gs.bounce ) {
    gs.direction.x *= -1.0f;
  }
  if ( rs.position.y > gs.bounce || rs.position.y < -gs.bounce ) {
    gs.direction.y *= -1.0f;
  }
  Ogre::Vector2 delta = gs.speed * ( (float)GAME_DELAY / 1000.0f ) * gs.direction;
  if ( !gs.mouse_pressed ) {
    if ( gs.rotation_speed.valueDegrees() == 0.0f ) {
      rs.position.x += delta.x;
      rs.position.y += delta.y;
    }
    //    printf( "game tick position: %f %f\n", rs.position.x, rs.position.y );

    // update the orientation of the head on a free roll
    // gs.rotation is unit length
    // gs.rotation_speed is in degrees/seconds
    // NOTE: sinf/cosf really needed there?
    gs.rotation_speed *= 0.97f;
    if ( gs.rotation_speed.valueDegrees() < 20.f ) {
      gs.rotation_speed = 0.0f;
    }
    float factor = sinf( 0.5f * Ogre::Degree( gs.rotation_speed * GAME_TICK_FLOAT ).valueRadians() );
    Ogre::Quaternion rotation_tick(
      cosf( 0.5f * Ogre::Degree( gs.rotation_speed * GAME_TICK_FLOAT ).valueRadians() ),
      factor * gs.rotation.x,
      factor * gs.rotation.y,
      factor * gs.rotation.z );
    rs.orientation = rotation_tick * rs.orientation;
  } else {
    // keep updating the orientation in the render state, even while the render thread is ignoring it:
    // when the game thread resumes control of the head orientation, it will interpolate from one of these states,
    // so we keep updating the orientation to avoid a short glitch at the discontinuity
    rs.orientation = orientation;
  }
}

int game_thread( void * _parms ) {
  GameThreadParms * parms = (GameThreadParms*)_parms;

  GameState gs;
  SharedRenderState rs;

  game_init( gs, rs );

  gs.zmq_control_socket = zsocket_new( parms->zmq_context, ZMQ_PAIR );
  {
    int ret = zsocket_connect( gs.zmq_control_socket, "inproc://control_game" );
    assert( ret == 0 );
  }
  
  gs.zmq_render_socket = zsocket_new( parms->zmq_context, ZMQ_PAIR );
  zsocket_bind( gs.zmq_render_socket, "inproc://game_render" );

  gs.zmq_input_req = zsocket_new( parms->zmq_context, ZMQ_REQ );
  {
    int ret = zsocket_connect( gs.zmq_input_req, "inproc://input" );
    assert( ret == 0 );
  }

  unsigned int baseline = SDL_GetTicks();
  int framenum = 0;
  while ( true ) {
    unsigned int now = SDL_GetTicks();
    unsigned int target_frame = ( now - baseline ) / GAME_DELAY;
    if ( framenum <= target_frame ) {
      framenum++;
      // NOTE: build the state of the world at t = framenum * GAME_DELAY,
      // under normal conditions that's a time in the future
      // (the exception to that is if we are catching up on ticking game frames)
      game_tick( now, gs, rs );
      // notify the render thread that a new game state is ready.
      // on the next render frame, it will start interpolating between the previous state and this new one
      zstr_sendf( gs.zmq_render_socket, "%d %f %f %f %f %f %f %f %f %f", baseline + framenum * GAME_DELAY, rs.position.x, rs.position.y, rs.orientation.w, rs.orientation.x, rs.orientation.y, rs.orientation.z, rs.smoothed_angular.x, rs.smoothed_angular.y, rs.smoothed_angular.z );
    } else {
      int ahead = framenum * GAME_DELAY - ( now - baseline );
      assert( ahead > 0 );
      printf( "game sleep %d ms\n", ahead );
      SDL_Delay( ahead );
    }
    char * cmd = zstr_recv_nowait( gs.zmq_control_socket );
    if ( cmd != NULL ) {
      assert( strcmp( cmd, "stop" ) == 0 );
      free( cmd );
      break;
    }
  }
  return 0;
}
