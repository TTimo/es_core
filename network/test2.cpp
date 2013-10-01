#include <stdio.h>
#include <assert.h>
#include <math.h>

#include <boost/asio.hpp>

#include "api.h"
#include "test_shared.h"

// TODO: implement a TCPSimpleRemotePeer

using namespace boost::asio;

int main( const int argc, const char *argv[] ) {
  io_service io;
  if ( argc > 1 ) {
    printf( "connecting to %s\n", argv[1] );
    // TODO
    return 0;
  }
  ip::tcp::socket socket( io );
  // TODO
  return 0;
}
