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

#ifndef _GAME_H_
#define _GAME_H_

const int ORIENTATION_LOG = 10;

typedef struct OrientationHistory_s {
  uint32_t t;
  Ogre::Quaternion o;
} OrientationHistory;

typedef struct GameState_s {
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

void game_init( GameThreadSockets & gsockets, GameState & gs, SharedRenderState & rs );
void game_tick( GameThreadSockets & gsockets, GameState & gs, SharedRenderState & rs, unsigned int now );
void emit_render_state( void * socket, unsigned int time, SharedRenderState & rs );

#endif
