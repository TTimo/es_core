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

// support functions, internal render state

typedef struct RenderState_s {
  bool mouse_control;
} RenderState;

void parse_mouse_state( char * mouse_state, Ogre::Quaternion & orientation ) { //, Ogre::Vector3 & rotation_vector ) {
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
  // NOTE: skipping the button state
}

void parse_render_state( char * render_state, int & tick_time, float & x, float & y, Ogre::Quaternion & orientation, Ogre::Vector3 & smoothed_angular ) {
  char * start = render_state;
  char * end = strchr( start, ' ' );
  end[0] = '\0';
  tick_time = atoi( start );
  start = end + 1;
  end = strchr( start, ' ' );
  end[0] = '\0';
  x = atof( start );
  start = end + 1;
  end = strchr( start, ' ' );
  end[0] = '\0';
  y = atof( start );
  start = end + 1;
  end = strchr( start, ' ' );
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
  end = strchr( start, ' ' );
  end[0] = '\0';
  smoothed_angular.x = atof( start );
  start = end + 1;
  end = strchr( start, ' ' );
  end[0] = '\0';
  smoothed_angular.y = atof( start );
  start = end + 1;
  smoothed_angular.z = atof( start );
}

// init and scene setup block

  rs.mouse_control = false;

  // create a test scene
  Ogre::ResourceGroupManager &mgr = Ogre::ResourceGroupManager::getSingleton();

  mgr.addResourceLocation( "media/models", "FileSystem", "General" );
  mgr.addResourceLocation( "media/materials/scripts", "FileSystem", "General" );
  mgr.addResourceLocation( "media/materials/textures", "FileSystem", "General" );
  mgr.addResourceLocation( "media/materials/programs", "FileSystem", "General" );

  mgr.initialiseAllResourceGroups();
    
  Ogre::SceneManager * scene = parms->root->createSceneManager( Ogre::ST_GENERIC, "SimpleStaticCheck" );
  scene->setAmbientLight( Ogre::ColourValue( 0.5f, 0.5f, 0.5f ) );
  Ogre::Entity * head = scene->createEntity( "head", "ogrehead.mesh" );
  Ogre::SceneNode * head_node = scene->getRootSceneNode()->createChildSceneNode( "head_node" );
  head_node->attachObject( head );

  Ogre::Light * light = scene->createLight( "light" );
  light->setPosition( 20.0f, 80.0f, 50.0f );
  
  Ogre::Camera * camera = scene->createCamera( "cam" );
  camera->setPosition( Ogre::Vector3( 0, 0, 90 ) );
  camera->lookAt( Ogre::Vector3( 0, 0, -300 ) );
  camera->setNearClipDistance( 5 );
  
  Ogre::Viewport * viewport = parms->ogre_window->addViewport( camera );
  viewport->setBackgroundColour( Ogre::ColourValue( 0, 0, 0 ) );
  camera->setAspectRatio( Ogre::Real( viewport->getActualWidth() ) / Ogre::Real( viewport->getActualHeight() ) );

  Ogre::ManualObject * rotation_vector_obj = scene->createManualObject( "rotation_vector" );
  rotation_vector_obj->setDynamic( true );
  rotation_vector_obj->begin( "BaseWhiteNoLighting", Ogre::RenderOperation::OT_LINE_LIST );
  rotation_vector_obj->position( 0.0f, 0.0f, 0.0f );
  rotation_vector_obj->position( 0.0f, 0.0f, 0.0f );
  rotation_vector_obj->end();
  Ogre::SceneNode * rotation_vector_node = scene->getRootSceneNode()->createChildSceneNode( "rotation_vector_node" );
  rotation_vector_node->attachObject( rotation_vector_obj );
  rotation_vector_node->setVisible( false );

// parse render state

      if ( game_tick[0] == '#' ) {
	// using '#' to decorate the head tracking trigger
	if ( game_tick[2] == '0' ) {
	  rs.mouse_control = false;
	  rotation_vector_node->setVisible( false );
	} else {
	  assert( game_tick[2] == '1' );
	  rs.mouse_control = true;
          // NOTE: uncomment to visualize the rotation vector
	  // rotation_vector_node->setVisible( true );
	}
        // wait for the next game state before continuing so we don't do renders with an inconsistent state
        free( game_tick );
        game_tick = zstr_recv( zmq_game_socket );
      }

      int tick_time;
      float x, y;
      Ogre::Quaternion orientation;
      Ogre::Vector3 smoothed_angular;
      parse_render_state( game_tick, tick_time, x, y, orientation, smoothed_angular );
      //printf( "render state head orientation: %f %f %f %f\n", orientation.w, orientation.x, orientation.y, orientation.z );
      free( game_tick );
      if ( previous_game_time == 0 ) {
	previous_game_time = tick_time;
	previous_render.position.x = x;
	previous_render.position.y = y;
	previous_render.position.z = 0.0f;
	previous_render.orientation = orientation;
	previous_render.smoothed_angular = smoothed_angular;
      } else {
	if ( next_game_time != 0 ) {	  
	  previous_game_time = next_game_time;
	  previous_render = next_render;
	}
	next_game_time = tick_time;
	next_render.position.x = x;
	next_render.position.y = y;
	next_render.position.z = 0.0f;
	next_render.orientation = orientation;
	next_render.smoothed_angular = smoothed_angular;
      }

// interpolate the render state


    interpolated_render.position = ( 1.0f - ratio ) * previous_render.position + ratio * next_render.position;
    interpolated_render.orientation = Ogre::Quaternion::Slerp( ratio, previous_render.orientation, next_render.orientation );
    interpolated_render.smoothed_angular = ( 1.0f - ratio ) * previous_render.smoothed_angular + ratio * next_render.smoothed_angular;
    head_node->setPosition( interpolated_render.position );
    if ( rs.mouse_control ) {
      zstr_send( zmq_input_req, "mouse_state" );
      char * mouse_state = zstr_recv( zmq_input_req );
      Ogre::Quaternion orientation;
      parse_mouse_state( mouse_state, orientation );

      // orient the head according to latest input data
      head_node->setOrientation( orientation );

      // update the rotation axis of the head (smoothed over a few frames in the game thread)
      rotation_vector_obj->beginUpdate( 0 );
      rotation_vector_obj->position( interpolated_render.position );
      Ogre::Vector3 rotation_vector_end = interpolated_render.position + 40.0f * interpolated_render.smoothed_angular;
      rotation_vector_obj->position( rotation_vector_end );
      rotation_vector_obj->end();

      free( mouse_state );
    } else {
      head_node->setOrientation( interpolated_render.orientation );
    }
