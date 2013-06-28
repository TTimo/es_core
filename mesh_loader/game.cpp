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

#include "czmq.h"

#include "shared_render_state.h"
#include "game.h"

void game_init( GameState & gs, SharedRenderState & rs ) {
  rs.position = Ogre::Vector3( 4000.0f, 4000.0f, -3000.0f );
}

void game_tick( unsigned int now, GameState & gs, SharedRenderState & rs ) {

  // parse input state: mouse orientation and keys
  // wasd for h. movement, space to go up, alt to go down
  // keep right mouse pressed to slow down, left mouse to move faster

  // support free fly

  rs.position = Ogre::Vector3( 4000.0f, 4000.0f, -3000.0f );
}

void emit_render_state( void * socket, unsigned int time, SharedRenderState & rs ) {
  zstr_sendf( socket, "%d", time );
}
