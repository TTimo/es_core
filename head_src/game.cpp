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

#include "OgreRoot.h"

#include "../game_main.h"

#include "shared_render_state.h"
#include "game.h"

void parse_mouse_state( char * mouse_state, Ogre::Quaternion & orientation, uint8_t & buttons ) {
  Ogre::Vector3 rotation_vector; // skipped over
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

void game_init( GameThreadSockets & gsockets, GameState & gs, SharedRenderState & rs ) {
  time_t now;
  time( &now );
  srandom( (unsigned int)now ); 
  gs.bounce = 25.0f;
  gs.speed = (float)( rand() % 60 + 40 );
  float angle = (float)( rand() % 360 ) * 2.0f * (float)M_PI / 360.0f;
  gs.direction = Ogre::Vector2( cosf( angle ), sinf( angle ) );
  rs.orientation = Ogre::Quaternion( Ogre::Radian( 0.0f ), Ogre::Vector3::UNIT_Z );
  rs.position = Ogre::Vector3::ZERO;
  gs.mouse_pressed = false;
  gs.rotation = Ogre::Vector3::UNIT_X;
  gs.rotation_speed = 0.0f;
  gs.orientation_index = 0;
  memset( gs.orientation_history, 0, sizeof( gs.orientation_history ) );
  // set the input code to manipulate an object rather than look around
  zstr_send( gsockets.zmq_input_req, "config_look_around 0" );
  char * foo = zstr_recv( gsockets.zmq_input_req );
  free( foo );
}

void game_tick( GameThreadSockets & gsockets, GameState & gs, SharedRenderState & srs, unsigned int now ) {
  // get the latest mouse buttons state and orientation
  zstr_send( gsockets.zmq_input_req, "mouse_state" );
  char * mouse_state = zstr_recv( gsockets.zmq_input_req );
  uint8_t buttons;
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
  uint32_t q1_t = gs.orientation_history[ q1_index ].t;
  Ogre::Quaternion omega = 2.0f * ( orientation - q1 ) * q1.UnitInverse() * ( 1000.0f / (float)( now - q1_t ) );
  omega.ToAngleAxis( gs.smoothed_angular_velocity, gs.smoothed_angular );
  //  printf( "%f %f %f - %f\n", gs.smoothed_angular.x, gs.smoothed_angular.y, gs.smoothed_angular.z, gs.smoothed_angular_velocity.valueDegrees() );
  srs.smoothed_angular = gs.smoothed_angular;

  if ( ( buttons & SDL_BUTTON( 1 ) ) != 0 ) {
    if ( !gs.mouse_pressed ) {
      gs.mouse_pressed = true;
      // changing the control scheme: the player is now driving the orientation of the head directly with the mouse
      // tell the input logic to reset the orientation to match the current orientation of the head
      zstr_sendf( gsockets.zmq_input_req, "mouse_reset %f %f %f %f", srs.orientation.w, srs.orientation.x, srs.orientation.y, srs.orientation.z );
      zstr_recv( gsockets.zmq_input_req ); // wait for ack from input
      // IF RENDER TICK HAPPENS HERE: render will not know that it should grab the orientation directly from the mouse,
      // but the orientation coming from game should still be ok?
      zstr_sendf( gsockets.zmq_render_socket, "# %s", "1" );
      // IF RENDER TICK HAPPENS HERE (before a new gamestate):
      // the now reset input orientation will combine with the old game state, that's bad
    }
  } else {
    if ( gs.mouse_pressed ) {
      gs.mouse_pressed = false;
      // changing the control scheme: the head will free spin and slow down for a bit, then it will resume bouncing around
      // the player looses mouse control, the game grabs latest orientation and angular velocity
      // the input thread was authoritative on orientation until now, so accept that as our starting orientation
      srs.orientation = orientation;
      gs.rotation_speed = gs.smoothed_angular_velocity;
      gs.rotation = gs.smoothed_angular;
      zstr_sendf( gsockets.zmq_render_socket, "# %s", "0" );
      // IF RENDER TICK HAPPENS HERE (before a new gamestate): render will pull the head orientation from the game state rather than input, but game state won't have the fixed orientation yet
    }
  }

  if ( srs.position.x > gs.bounce || srs.position.x < -gs.bounce ) {
    gs.direction.x *= -1.0f;
  }
  if ( srs.position.y > gs.bounce || srs.position.y < -gs.bounce ) {
    gs.direction.y *= -1.0f;
  }
  Ogre::Vector2 delta = gs.speed * ( (float)GAME_DELAY / 1000.0f ) * gs.direction;
  if ( !gs.mouse_pressed ) {
    if ( gs.rotation_speed.valueDegrees() == 0.0f ) {
      srs.position.x += delta.x;
      srs.position.y += delta.y;
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
    srs.orientation = rotation_tick * srs.orientation;
  } else {
    // keep updating the orientation in the render state, even while the render thread is ignoring it:
    // when the game thread resumes control of the head orientation, it will interpolate from one of these states,
    // so we keep updating the orientation to avoid a short glitch at the discontinuity
    srs.orientation = orientation;
  }
}

void emit_render_state( void * socket, unsigned int time, SharedRenderState & srs ) {
  zstr_sendf( socket, "%d %f %f %f %f %f %f %f %f %f", time, srs.position.x, srs.position.y, srs.orientation.w, srs.orientation.x, srs.orientation.y, srs.orientation.z, srs.smoothed_angular.x, srs.smoothed_angular.y, srs.smoothed_angular.z );
}
