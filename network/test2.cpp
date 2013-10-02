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
    // client
    try {
      printf( "Connecting to %s\n", argv[1] );
      ip::tcp::resolver resolver( io );
      // NOTE: haven't seen a way to parse a URI, only IP and port/service needs to be given separately
      ip::tcp::resolver::query query( ip::tcp::v4(), argv[1], "8080" );
      ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve( query );
      // assume a single endpoint
      ip::tcp::endpoint remote = *endpoint_iterator;    
      ip::tcp::resolver::iterator end;
      endpoint_iterator++;
      assert( endpoint_iterator == end );
      ip::tcp::socket socket( io );
      socket.connect( remote );
      printf( "Connected\n" );
    } catch ( std::exception &e ) {
      std::cerr << e.what() << std::endl;
      return 1;
    }
    return 0;
  }
  // server
  try {
    ip::tcp::acceptor acceptor( io, ip::tcp::endpoint( ip::tcp::v4(), 8080 ) );
    printf( "Listening on port 8080\n" );
    ip::tcp::socket socket( io );
    acceptor.accept( socket );
    printf( "Accepted connection\n" );
  } catch ( std::exception &e ) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
  return 0;
}
