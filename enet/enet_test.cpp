#include <stdio.h>
#include <enet/enet.h>

int main( int argc, char * argv[] ) {
  if ( enet_initialize() != 0 ) {
    fprintf( stderr, "An error occurred while initializing ENet.\n" );
    return EXIT_FAILURE;
  }
  atexit( enet_deinitialize );
}
