#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "api.h"
#include "test_shared.h"

class LocalSimpleRemotePeer : public SimpleRemotePeer {
  float v;
  bool read_ready;
public:
  LocalSimpleRemotePeer() : v(0.0f), read_ready(false) { }

  /*
   * SimpleRemotePeer
   */

  void WriteFloat( float & _v ) {
    assert( !read_ready ); // means the previous write value would be squashed without ever being read
    v = _v;
    read_ready = true;
  }
  bool ReadFloat( float & _v ) {
    if ( !read_ready ) {
      return false;
    }
    _v = v;
    read_ready = false;
    return true;
  }

  /*
   * Internal
   */

  void DoTransport( SimpleRemotePeer &remote ) {
    // mimics what happens if two peers actually talk to each other (across threads, TCP, UDP etc.)
    remote.WriteFloat( v );
    read_ready = false;
  }

};

int main( const int argc, const char *argv[] ) {
  ScalarObject o;
  SimpleNode server;
  server.AddObject( &o );

  ScalarProxy p;  
  SimpleNode client;
  client.AddProxy( &p );

  LocalSimpleRemotePeer c2s;
  LocalSimpleRemotePeer s2c;
  client.AddPeer( &c2s );
  server.AddPeer( &s2c );

  unsigned int snapshot_accu = SNAPSHOT_FRAMES;
  while ( server.GetTime() < 30000 ) {
    snapshot_accu -= 1;
    server.Tick();
    if ( snapshot_accu <= 0 ) {
      server.WriteState();
      s2c.DoTransport( c2s );
      snapshot_accu = SNAPSHOT_FRAMES;
    }
    client.Tick();
  }

  return 0;
}
