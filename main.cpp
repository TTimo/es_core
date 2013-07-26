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

#include "SDL.h"
#include "SDL_opengl.h"
#include "SDL_syswm.h"
#include "SDL_thread.h"
// bad SDL, bad.. seems to be Windows only at least
#ifdef main
#undef main
#endif

#include "OgreRoot.h"
#include "OgreRenderWindow.h"

#include "czmq.h"

#ifdef __APPLE__
  #include <syslog.h>
  #include "OSX_wrap.h"
#endif

#include "render_main.h"
#include "game_main.h"

typedef struct InputState_s {
  float yaw_sens;
  float pitch_sens;
  float orientation_factor;	// +1/-1 easy switch between look around and manipulate something
  float yaw;			// degrees, modulo ]-180,180] range
  float pitch;			// degrees, clamped [-90,90] range
  float roll;
  Ogre::Quaternion orientation;	// current orientation  
} InputState;

void parse_orientation( char * start, Ogre::Quaternion & orientation ) {
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
  orientation.z = (float)atof( start );
}

void send_shutdown( void * zmq_render_socket, void * zmq_game_socket ) {
  zstr_send( zmq_render_socket, "stop" );
  zstr_send( zmq_game_socket, "stop" );
}

void wait_shutdown( SDL_Thread * & sdl_render_thread, SDL_Thread * & sdl_game_thread, void * zmq_input_rep ) {
  // there is no timeout support in SDL_WaitThread, so we can only call it once we're sure the thread is going to finish
  // the threads may do a few polls against the input thread before actually shutting down
  // TODO: I'd like to come up with a more robust design:
  // - thinking each thread should have a single select for all it's sockets, but that may not be practical
  // - or add a control socket from each of these threads back to the main thread, so we can wait until the threads confirm that they are shutting down
  // for now, loop the input thread for a bit to flush out any events
  Uint32 continue_time = SDL_GetTicks() + 500; // an eternity
  while ( SDL_GetTicks() < continue_time ) {
    char * req = zstr_recv_nowait( zmq_input_rep );
    if ( req != NULL ) {
      delete( req );
      // send a nop - that's assuming that all the code interacting with the input socket knows how to handle an empty response,
      // which isn't the case, so the parsing code might crash .. still better than a hang
      zstr_send( zmq_input_rep, "" );
    } else {
      SDL_Delay( 10 );
    }
  }
  {
    int status;
    SDL_WaitThread( sdl_render_thread, &status );
    printf( "render thread has stopped, status: %d\n", status );
    sdl_render_thread = NULL;
  }
  {
    int status;
    SDL_WaitThread( sdl_game_thread, &status );
    printf( "game thread has stopped, status: %d\n", status );
    sdl_game_thread = NULL;
  }
}

int main( int argc, char *argv[] ) {
  int retcode = 0;

#ifdef __APPLE__
  // set the working directory to the top directory of the bundle
  struct stat st;
  if ( stat( "media", &st ) != 0 ) {
    //    printf( "%s\n", argv[0] );    
    syslog( LOG_ALERT, "%s\n", argv[0] );
    char bindir[PATH_MAX];
    strcpy( bindir, argv[0] );
    char * stop = strrchr( bindir, '/' );
    *stop = '\0';
    char cwd[PATH_MAX];
    sprintf( cwd, "%s/../../..", bindir );
    //    printf( "chdir %s\n", buf );
    syslog( LOG_ALERT, "chdir %s\n", cwd );
    chdir( cwd );
    assert( stat( "media", &st ) == 0 );
  }  
#endif

  SDL_Init( SDL_INIT_EVERYTHING );
  SDL_Window * window = SDL_CreateWindow(
					 "es_core::SDL",
					 SDL_WINDOWPOS_UNDEFINED,
					 SDL_WINDOWPOS_UNDEFINED,
					 800,
					 600,
					 SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN/*|SDL_WINDOW_RESIZABLE*/ );
  if ( window == NULL ) {
    printf( "SDL_CreateWindow failed: %s\n", SDL_GetError() );
    return -1;
  }

  SDL_GLContext glcontext = NULL;
#ifndef __APPLE__
  glcontext = SDL_GL_CreateContext( window );
  if ( glcontext == NULL ) {
    printf( "SDL_GL_CreateContext failed: %s\n", SDL_GetError() );
    return -1;
  }
#endif
  SDL_SysWMinfo syswm_info;
  SDL_VERSION( &syswm_info.version );
  if ( !SDL_GetWindowWMInfo( window, &syswm_info ) ) {
    printf( "SDL_GetWindowWMInfo failed.\n" );
    return -1;
  }
  
  try {

    Ogre::Root * ogre_root = new Ogre::Root();
#ifdef __WINDOWS__
#ifdef _DEBUG
    ogre_root->loadPlugin( "RenderSystem_GL_d" );
#else
    ogre_root->loadPlugin( "RenderSystem_GL" );
#endif
#else
    ogre_root->loadPlugin( "RenderSystem_GL" );
#endif
    if ( ogre_root->getAvailableRenderers().size() != 1 ) {
      OGRE_EXCEPT( Ogre::Exception::ERR_INTERNAL_ERROR, "Failed to initialize RenderSystem_GL", "main" );
    }
    ogre_root->setRenderSystem( ogre_root->getAvailableRenderers()[0] );
    ogre_root->initialise( false );

    Ogre::NameValuePairList params;
#ifdef __WINDOWS__
    params["externalGLControl"] = "1";
    // only supported for Win32 on Ogre 1.8 not on other platforms (documentation needs fixing to accurately reflect this)
    params["externalGLContext"] = Ogre::StringConverter::toString( (unsigned long)glcontext );
    params["externalWindowHandle"] = Ogre::StringConverter::toString( (unsigned long)syswm_info.info.win.window );
#elif __LINUX__
    params["externalGLControl"] = "1";
    // not documented in Ogre 1.8 mainline, supported for GLX and EGL{Win32,X11}
    params["currentGLContext"] = "1";
    // NOTE: externalWindowHandle is reported as deprecated (GLX Ogre 1.8)
    params["parentWindowHandle"] = Ogre::StringConverter::toString( (unsigned long)syswm_info.info.x11.window );
#elif __APPLE__
    params["externalGLControl"] = "1";
    // only supported for Win32 on Ogre 1.8 not on other platforms (documentation needs fixing to accurately reflect this)
    //    params["externalGLContext"] = Ogre::StringConverter::toString( glcontext );
    params["externalWindowHandle"] = OSX_cocoa_view( syswm_info );
    params["macAPI"] = "cocoa";
    params["macAPICocoaUseNSView"] = "true";
#endif
    Ogre::RenderWindow * ogre_render_window = ogre_root->createRenderWindow( "Legion::core::ogre", 800, 600, false, &params );
#ifdef __APPLE__
    // I suspect triple buffering is on by default, which makes vsync pointless?
    // except maybe for poorly implemented render loops which will then be forced to wait
    //    ogre_render_window->setVSyncEnabled( false );
#else
    // NOTE: SDL_GL_SWAP_CONTROL was SDL 1.2 and has been retired
    SDL_GL_SetSwapInterval( 1 );
#endif

    // NOTE: since we are driving with SDL, we need to keep the Ogre side updated for window visibility
    ogre_render_window->setVisible( true );

    zctx_t * zmq_context = zctx_new();

    void * zmq_game_socket = zsocket_new( zmq_context, ZMQ_PAIR );
    zsocket_bind( zmq_game_socket, "inproc://control_game" );
    
    void * zmq_render_socket = zsocket_new( zmq_context, ZMQ_PAIR );
    zsocket_bind( zmq_render_socket, "inproc://control_render" );

    void * zmq_input_rep = zsocket_new( zmq_context, ZMQ_REP );
    zsocket_bind( zmq_input_rep, "inproc://input" );

    GameThreadParms game_thread_parms;
    game_thread_parms.zmq_context = zmq_context;
    SDL_Thread * sdl_game_thread = SDL_CreateThread( game_thread, "game", &game_thread_parms );

    RenderThreadParms render_thread_parms;
    render_thread_parms.root = ogre_root;
    render_thread_parms.window = window;
    render_thread_parms.gl_context = glcontext;
    render_thread_parms.ogre_window = ogre_render_window;
    render_thread_parms.zmq_context = zmq_context;
    render_thread_parms.argc = argc;
    render_thread_parms.argv = argv;
#ifdef __APPLE__
    OSX_GL_clear_current( ogre_render_window );
#else
    // NOTE: don't call this on OSX, since SDL isn't managing the context!
    SDL_GL_MakeCurrent( window, NULL );
#endif
    SDL_Thread * sdl_render_thread = SDL_CreateThread( render_thread, "render", &render_thread_parms );

    SDL_SetWindowGrab( window, SDL_TRUE );
    SDL_SetRelativeMouseMode( SDL_TRUE );
  
    //    const int MAX_RUN_TIME = 3 * 60 * 1000; // shutdown the whole thing after some time
    bool shutdown_requested = false;
    InputState is;
    is.yaw_sens = 0.1f;
    is.yaw = 0.0f;
    is.pitch_sens = 0.1f;
    is.pitch = 0.0f;
    is.roll = 0.0f;
    is.orientation_factor = -1.0f; // look around config
    while ( !shutdown_requested /* && SDL_GetTicks() < MAX_RUN_TIME */ ) {
      // we wait here
      char * input_request = zstr_recv( zmq_input_rep );
      // poll for events before processing the request
      // NOTE: this is how SDL builds the internal mouse and keyboard state
      // TODO: done this way does not meet the objectives of smooth, frame independent mouse view control,
      //   plus it throws some latency into the calling thread
      SDL_Event event;
      while ( SDL_PollEvent( &event ) ) {
	if ( event.type == SDL_KEYDOWN || event.type == SDL_KEYUP ) {
	  printf( "SDL_KeyboardEvent\n" );
	  if ( event.type == SDL_KEYUP && ((SDL_KeyboardEvent*)&event)->keysym.scancode == SDL_SCANCODE_ESCAPE ) {
	    send_shutdown( zmq_render_socket, zmq_game_socket );
	    shutdown_requested = true;	    
	  }
	} else if ( event.type == SDL_MOUSEMOTION ) {
	  SDL_MouseMotionEvent * mev = (SDL_MouseMotionEvent*)&event;
	  // + when manipulating an object, - when doing a first person view .. needs to be configurable?
	  is.yaw += is.orientation_factor * is.yaw_sens * (float)mev->xrel;
	  if ( is.yaw >= 0.0f ) {
	    is.yaw = fmod( is.yaw + 180.0f, 360.0f ) - 180.0f;
	  } else {
	    is.yaw = fmod( is.yaw - 180.0f, 360.0f ) + 180.0f;
	  }
	  // + when manipulating an object, - when doing a first person view .. needs to be configurable?
	  is.pitch += is.orientation_factor * is.pitch_sens * (float)mev->yrel;
	  if ( is.pitch > 90.0f ) {
	    is.pitch = 90.0f;
	  } else if ( is.pitch < -90.0f ) {
	    is.pitch = -90.0f;
	  }
	  // build a quaternion of the current orientation
	  Ogre::Matrix3 r;
	  r.FromEulerAnglesYXZ( Ogre::Radian( Ogre::Degree( is.yaw ) ), Ogre::Radian( Ogre::Degree( is.pitch ) ), Ogre::Radian( Ogre::Degree( is.roll ) ) );
	  is.orientation.FromRotationMatrix( r );
	} else if ( event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEBUTTONDOWN ) {
	  printf( "SDL_MouseButtonEvent\n" );
	} else if ( event.type == SDL_QUIT ) {
	  printf( "SDL_Quit\n" );
	  // push a shutdown on the control socket, game and render will pick it up later
	  // NOTE: if the req/rep patterns change we may still have to deal with hangs here
	  send_shutdown( zmq_render_socket, zmq_game_socket );
	  shutdown_requested = true;
	} else {
	  printf( "SDL_Event %d\n", event.type );
	}
      }
      // we are ready to process the request now
      if ( strcmp( input_request, "mouse_state" ) == 0 ) {
	int x, y;
	Uint8 buttons = SDL_GetMouseState( &x, &y );
	zstr_sendf( zmq_input_rep, "%f %f %f %f %d", is.orientation.w, is.orientation.x, is.orientation.y, is.orientation.z, buttons );
      } else if ( strcmp( input_request, "kb_state" ) == 0 ) {
	// looking at a few hardcoded keys for now
	// NOTE: I suspect it would be perfectly safe to grab that pointer once, and read it from a different thread?
	const Uint8 *state = SDL_GetKeyboardState(NULL);
	zstr_sendf( zmq_input_rep, "%d %d %d %d %d %d", state[ SDL_SCANCODE_W ], state[ SDL_SCANCODE_A ], state[ SDL_SCANCODE_S ], state[ SDL_SCANCODE_D ], state[ SDL_SCANCODE_SPACE ], state[ SDL_SCANCODE_LALT ] );
      } else if ( strncmp( input_request, "mouse_reset", strlen( "mouse_reset" ) ) == 0 ) {
	// reset the orientation
	parse_orientation( input_request + strlen( "mouse_reset" ) + 1, is.orientation );

	Ogre::Matrix3 r;
	is.orientation.ToRotationMatrix( r );
	Ogre::Radian rfYAngle, rfPAngle, rfRAngle;
	r.ToEulerAnglesYXZ( rfYAngle, rfPAngle, rfRAngle );
	is.yaw = rfYAngle.valueDegrees();
	is.pitch = rfPAngle.valueDegrees();
	is.roll = rfRAngle.valueDegrees();

	zstr_send( zmq_input_rep, "" ); // nop (acknowledge)
      } else if ( strncmp( input_request, "config_look_around", strlen( "config_look_around" ) ) == 0 ) {
	if ( atoi( input_request + strlen( "config_look_around" ) + 1 ) == 0 ) {
	  printf( "input configuration: manipulate object\n" );
	  is.orientation_factor = 1.0f;
	} else {
	  printf( "input configuration: look around\n" );
	  is.orientation_factor = -1.0f;
	}
	zstr_send( zmq_input_rep, "" ); // nop
      } else {
	zstr_send( zmq_input_rep, "" ); // nop
      }
      free( input_request );
    }

    if ( !shutdown_requested ) {
      send_shutdown( zmq_render_socket, zmq_game_socket );
      shutdown_requested = true;
    }
    wait_shutdown( sdl_render_thread, sdl_game_thread, zmq_input_rep );

    zctx_destroy( &zmq_context );
    // make the GL context again before proceeding with the teardown
#ifdef __APPLE__
    OSX_GL_set_current( ogre_render_window );
#else
    SDL_GL_MakeCurrent( window, glcontext );
#endif
    delete ogre_root;

  } catch ( Ogre::Exception &e ) {
    printf( "Ogre::Exception %s\n", e.what() );
    retcode = -1;
  }

#ifndef __WINDOWS__
  // FIXME: Windows crashes in the OpenGL drivers inside the DestroyWindow() call ?
  // (is that related to the main redefine shenanigans?)
  SDL_Quit();
#endif

  return retcode;
}
