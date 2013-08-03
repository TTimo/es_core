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

void render_init( RenderThreadParms * parms, RenderState & rs, SharedRenderState & srs ) {

  Ogre::ResourceGroupManager &mgr = Ogre::ResourceGroupManager::getSingleton();

  mgr.addResourceLocation( "media/models", "FileSystem", "General" );
  mgr.addResourceLocation( "media/materials/scripts", "FileSystem", "General" );
  mgr.addResourceLocation( "media/materials/textures", "FileSystem", "General" );
  mgr.addResourceLocation( "media/materials/programs", "FileSystem", "General" );

  mgr.initialiseAllResourceGroups();

  Ogre::SceneManager * scene = parms->root->createSceneManager( Ogre::ST_GENERIC, "SimpleStaticCheck" );
  // this modulates the material's ambient value .. edit the material
  scene->setAmbientLight( Ogre::ColourValue( 1.0f, 1.0f, 1.0f ) );

  Ogre::Entity * model;
  if ( parms->argc > 1 ) {
    printf( "loading model: %s\n", parms->argv[1] );
    model = scene->createEntity( "model", parms->argv[1] );
  } else {
    model = scene->createEntity( "model", "uptown.mesh" );
  }
  model->setMaterialName( "es_core/flat" );
  Ogre::SceneNode * node = scene->getRootSceneNode()->createChildSceneNode( "model_node" );
  node->attachObject( model );

  // put the model in the ZX plane
  node->setPosition( Ogre::Vector3::ZERO );
  node->pitch( Ogre::Radian( Ogre::Degree( 90.0f ) ), Ogre::Node::TS_WORLD );

  rs.light = scene->createLight( "light" );
  rs.light->setType( Ogre::Light::LT_DIRECTIONAL );
  // this interacts with the diffuse in the material in ways that I don't completely understand
  rs.light->setDiffuseColour( 0.6f, 0.6f, 0.6f );

  rs.camera = scene->createCamera( "cam" );
  rs.camera->setNearClipDistance( 5 );

  Ogre::Viewport * viewport = parms->ogre_window->addViewport( rs.camera );
  viewport->setBackgroundColour( Ogre::ColourValue( 0, 0, 0 ) );
  rs.camera->setAspectRatio( Ogre::Real( viewport->getActualWidth() ) / Ogre::Real( viewport->getActualHeight() ) );
}

void parse_render_state( RenderState & rs, SharedRenderState & srs, char * msg ) {
  char * start = msg;
  char * end = strchr( start, ' ' );
  end[0] = '\0';
  srs.game_time = atoi( start );
  start = end + 1;
  end = strchr( start, ' ' );
  end[0] = '\0';
  srs.position[0] = (float)atof( start );
  start = end + 1;
  end = strchr( start, ' ' );
  end[0] = '\0';
  srs.position[1] = (float)atof( start );
  start = end + 1;
  srs.position[2] = (float)atof( start );  
}

void interpolate_and_render( RenderThreadSockets & rsockets, RenderState & rs, float ratio, SharedRenderState & previous_render, SharedRenderState & next_render ) {
  zstr_send( rsockets.zmq_input_req, "mouse_state" );
  char * mouse_state = zstr_recv( rsockets.zmq_input_req );
  Ogre::Quaternion o;
  parse_mouse_state( mouse_state, o );
  free( mouse_state );

  rs.camera->setOrientation( o );
  
  // with the flat shader and the directional this helps looking at the geometry
  rs.light->setDirection( rs.camera->getDirection() );

  Ogre::Vector3 v = ( 1.0f - ratio ) * previous_render.position + ratio * next_render.position;

  rs.camera->setPosition( v );
}
