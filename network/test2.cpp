#include <stdio.h>
#include <assert.h>
#include <math.h>

#include <boost/asio.hpp>

#include "api.h"
#include "test_shared.h"

using namespace boost::asio;

class TCPSimpleRemotePeer : public SimpleRemotePeer {
  ip::tcp::socket *socket;
public:
  TCPSimpleRemotePeer( ip::tcp::socket *_socket ) {
    socket = _socket;
  }

  /*
   * SimpleRemotePeer
   */

  void WriteFloat( float & _v ) {
    boost::system::error_code ignored_error;
    boost::asio::write( *socket, boost::asio::buffer( &_v, sizeof( _v ) ), boost::asio::transfer_all(), ignored_error );
  }
  bool ReadFloat( float & _v ) {
    boost::system::error_code error;
    size_t len = socket->read_some( boost::asio::buffer( &_v, sizeof( _v ) ), error );
    return ( len != 0 ); // read one float at a time, return true if there was something
  }

};

int main( const int argc, const char *argv[] ) {
  io_service io;
  deadline_timer timer( io );
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
      socket.non_blocking( true );
      printf( "Connected\n" );

      ScalarProxy p;
      SimpleNode client;
      client.AddProxy( &p );

      TCPSimpleRemotePeer peer( &socket );
      client.AddPeer( &peer );

      // NOTE: running till server socket closes on us
      // TODO: timing! have to add a tick speed
      while ( socket.is_open() ) {
	timer.expires_from_now( boost::posix_time::milliseconds( 16 ) );
	timer.wait();
	client.Tick();
      }

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
    // single peer
    acceptor.accept( socket );
    printf( "Accepted connection\n" );
    
    ScalarObject o;
    SimpleNode server;
    server.AddObject( &o );

    TCPSimpleRemotePeer peer( &socket );
    server.AddPeer( &peer );

    unsigned int snapshot_accu = SNAPSHOT_FRAMES;
    while ( server.GetTime() < 30000 ) {
      timer.expires_from_now( boost::posix_time::milliseconds( 16 ) );
      timer.wait();
      snapshot_accu -= 1;
      server.Tick();
      if ( snapshot_accu <= 0 ) {
	server.WriteState();
	snapshot_accu = SNAPSHOT_FRAMES;
      }      
    }
    printf( "Closing connection\n" );
    // FIXME: closing server side doesn't lead to the client side seeing the socket closed?
    socket.close();
    //    socket.shutdown( ip::tcp::socket::shutdown_both );
    acceptor.close();
  } catch ( std::exception &e ) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
  return 0;
}
