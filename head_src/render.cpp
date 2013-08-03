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
#include "OgreRenderWindow.h"
#include "OgreViewport.h"
#include "OgreEntity.h"
#include "OgreManualObject.h"

#include "../render_main.h"

#include "shared_render_state.h"
#include "render.h"

void parse_mouse_state( char * mouse_state, Ogre::Quaternion & orientation ) {
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
  // NOTE: skipping the button state
}

void _parse_render_state( char * render_state, unsigned int & tick_time, float & x, float & y, Ogre::Quaternion & orientation, Ogre::Vector3 & smoothed_angular ) {
  char * start = render_state;
  char * end = strchr( start, ' ' );
  end[0] = '\0';
  tick_time = atoi( start );
  start = end + 1;
  end = strchr( start, ' ' );
  end[0] = '\0';
  x = (float)atof( start );
  start = end + 1;
  end = strchr( start, ' ' );
  end[0] = '\0';
  y = (float)atof( start );
  start = end + 1;
  end = strchr( start, ' ' );
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
  end = strchr( start, ' ' );
  end[0] = '\0';
  smoothed_angular.x = (float)atof( start );
  start = end + 1;
  end = strchr( start, ' ' );
  end[0] = '\0';
  smoothed_angular.y = (float)atof( start );
  start = end + 1;
  smoothed_angular.z = (float)atof( start );
}

void render_init( RenderThreadParms * parms, RenderState & rs, SharedRenderState & srs ) {
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
  rs.head_node = scene->getRootSceneNode()->createChildSceneNode( "head_node" );
  rs.head_node->attachObject( head );

  Ogre::Light * light = scene->createLight( "light" );
  light->setPosition( 20.0f, 80.0f, 50.0f );
  
  Ogre::Camera * camera = scene->createCamera( "cam" );
  camera->setPosition( Ogre::Vector3( 0, 0, 90 ) );
  camera->lookAt( Ogre::Vector3( 0, 0, -300 ) );
  camera->setNearClipDistance( 5 );
  
  Ogre::Viewport * viewport = parms->ogre_window->addViewport( camera );
  viewport->setBackgroundColour( Ogre::ColourValue( 0, 0, 0 ) );
  camera->setAspectRatio( Ogre::Real( viewport->getActualWidth() ) / Ogre::Real( viewport->getActualHeight() ) );

  rs.rotation_vector_obj = scene->createManualObject( "rotation_vector" );
  rs.rotation_vector_obj->setDynamic( true );
  rs.rotation_vector_obj->begin( "BaseWhiteNoLighting", Ogre::RenderOperation::OT_LINE_LIST );
  rs.rotation_vector_obj->position( 0.0f, 0.0f, 0.0f );
  rs.rotation_vector_obj->position( 0.0f, 0.0f, 0.0f );
  rs.rotation_vector_obj->end();
  rs.rotation_vector_node = scene->getRootSceneNode()->createChildSceneNode( "rotation_vector_node" );
  rs.rotation_vector_node->attachObject( rs.rotation_vector_obj );
  rs.rotation_vector_node->setVisible( false );
}

void parse_render_state( RenderState & rs, SharedRenderState & srs, char * game_tick ) {
  if ( game_tick[0] == '#' ) {
    // using '#' to decorate the head tracking trigger
    if ( game_tick[2] == '0' ) {
      rs.mouse_control = false;
      rs.rotation_vector_node->setVisible( false );
    } else {
      assert( game_tick[2] == '1' );
      rs.mouse_control = true;
      // NOTE: uncomment to visualize the rotation vector
      rs.rotation_vector_node->setVisible( true );
    }
    // resume updating render state on the next loop
    return;
  }

  float x, y;
  _parse_render_state( game_tick, srs.game_time, x, y, srs.orientation, srs.smoothed_angular );
  srs.position.x = x;
  srs.position.y = y;
  srs.position.z = 0.0f;
}

void interpolate_and_render( RenderThreadSockets & rsockets, RenderState & rs, float ratio, SharedRenderState & previous_render, SharedRenderState & next_render ) {
  Ogre::Vector3 interp_position = ( 1.0f - ratio ) * previous_render.position + ratio * next_render.position;
  rs.head_node->setPosition( interp_position );
  if ( rs.mouse_control ) {
    zstr_send( rsockets.zmq_input_req, "mouse_state" );
    char * mouse_state = zstr_recv( rsockets.zmq_input_req );
    Ogre::Quaternion orientation;
    parse_mouse_state( mouse_state, orientation );
    free( mouse_state );
    // use latest mouse data to orient the head
    rs.head_node->setOrientation( orientation );
    // update the rotation axis of the head (smoothed over a few frames in the game thread)
    rs.rotation_vector_obj->beginUpdate( 0 );
    rs.rotation_vector_obj->position( interp_position );
    Ogre::Vector3 interp_smoothed_angular = ( 1.0f - ratio ) * previous_render.smoothed_angular + ratio * next_render.smoothed_angular;
    Ogre::Vector3 rotation_vector_end = interp_position + 40.0f * interp_smoothed_angular;
    rs.rotation_vector_obj->position( rotation_vector_end );
    rs.rotation_vector_obj->end();
  } else {
    rs.head_node->setOrientation( Ogre::Quaternion::Slerp( ratio, previous_render.orientation, next_render.orientation ) );
  }
}
