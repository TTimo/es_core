#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <enet/enet.h>

static const unsigned short LISTEN_PORT = 26000;

int main( int argc, char * argv[] ) {
  if ( enet_initialize() != 0 ) {
    fprintf( stderr, "An error occurred while initializing ENet.\n" );
    return EXIT_FAILURE;
  }
  atexit( enet_deinitialize );

  if ( argc >= 2 ) {
    ENetHost * client = enet_host_create( NULL, 32, 0, 0, 0 );
    if ( client == NULL ) {
      fprintf( stderr, "enet_host_create client failed\n" );
      exit( EXIT_FAILURE );      
    }
    ENetAddress address;
    enet_address_set_host( &address, argv[1] );
    address.port = LISTEN_PORT;
    ENetPeer * peer = enet_host_connect( client, &address, 32, 0 );
    if ( peer == NULL ) {
      fprintf( stderr, "enet_host_connect to %s failed\n", argv[1] );
      exit( EXIT_FAILURE );      
    }
    ENetEvent event;
    int ret = enet_host_service( client, &event, 5000 );
    if ( ret <= 0 ) {
      fprintf( stderr, "Connection failed.\n" );
      exit( EXIT_FAILURE );
    }
    printf( "Connected to %s\n", argv[1] );
    const char * hello = "Hello enet";
    ENetPacket * packet = enet_packet_create( hello, strlen( hello ) + 1, ENET_PACKET_FLAG_RELIABLE );
    enet_peer_send( peer, 0, packet );
    ret = enet_host_service( client, &event, 5000 );
  } else {
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = LISTEN_PORT;
    ENetHost * server = enet_host_create( &address, 32, 0, 0, 0 );
    if ( server == NULL ) {
      fprintf( stderr, "enet_host_create port %d failed\n", LISTEN_PORT );
      exit( EXIT_FAILURE );
    }
    printf( "Listening on port %d\n", LISTEN_PORT );
    while ( true ) {
      ENetEvent event;
      int ret = enet_host_service( server, &event, 1000 );
      if ( ret < 0 ) {
	fprintf( stderr, "enet_host_service returned error %d\n", ret );
	exit( EXIT_FAILURE ); // transient errors?
      } else if ( ret == 0 ) {
	continue; // no event, loop again
      }
      assert( ret > 0 );
      switch ( event.type ) {
      case ENET_EVENT_TYPE_CONNECT:
	printf( "Connection from %x:%u\n",
		event.peer->address.host,
		event.peer->address.port );
	break;
      case ENET_EVENT_TYPE_RECEIVE:
	printf( " %x:%u channel %u -> %s\n",
		event.peer->address.host,
		event.peer->address.port,
		event.channelID,
		event.packet->data );
	enet_packet_destroy( event.packet );
	break;
      case ENET_EVENT_TYPE_DISCONNECT:
	printf( " %x:%u disconnected\n",
		event.peer->address.host,
		event.peer->address.port );
	break;
      default:
	printf( "Unhandled event %d\n", event.type );
	break;
      }
    }
  }
  return 0;
}
