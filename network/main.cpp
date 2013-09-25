/*
 * API
 */

#include <stdio.h>
#include <math.h>

// message oriented communication with a remote peer
class RemotePeer {
};

// there is exactly one Object instance for each thing that exists in the distributed system
// if a node has an Object instance, then he is the 'server', e.g. authoritative
class Object {
public:
  virtual void WriteState( RemotePeer &peer ) = 0;
  virtual void Tick( unsigned int time ) = 0;
};

// a Proxy instance uniquely identifies a single Object instance in the distributed system
// if a node has a Proxy instance and the Object instance lives on another node, he's a 'client', e.g. non authoritative
class Proxy {
public:
  virtual void ReadState( RemotePeer &peer ) = 0;
  virtual void Tick( unsigned int time ) = 0;
};

// a Node instance holds Object and Proxy instances and ticks it's state:
// reading input to update the state of Object instances (authoritative)
// updating Proxy instances based on network input
class Node {
public:
  virtual void AddObject( Object * o ) = 0;
  virtual void AddProxy( Proxy * p ) = 0;
  virtual void AddPeer( RemotePeer * c ) = 0;
  virtual void Tick() = 0;
};

/*
 * Demo implementation
 */

// same process, no threading, just store and pass messages around
class SimpleRemotePeer : public RemotePeer {
  float v;
public:
  SimpleRemotePeer() : v(0.0f) { }

  /*
   * Internal
   */

  void DoTransport( SimpleRemotePeer &remote ) {    
    // TODO: manage some buffered transport delay (fixed delay, random fluctuations, etc.)
    remote.WriteFloat( v );
  }
  void WriteFloat( float & _v ) {
    v = _v;
  }
  void ReadFloat( float & _v ) {
    _v = v;
  }
};

class ScalarObject : public Object {
  float v;
public:
  ScalarObject() : v( 0.0f ) { }
  void WriteState( RemotePeer &peer ) {
    static_cast<SimpleRemotePeer&>(peer).WriteFloat( v );
  }
  void Tick( unsigned int time ) {
    // TODO: rectangular, triangle etc.
    // 1Hz sin signal
    v = sinf( (float)time * 2.0f * M_PI / 1000.0f );
    printf( "client: %d %f\n", time, v );
  }
};

class ScalarProxy : public Proxy {
  float v;
public:
  ScalarProxy() : v( 0.0f ) { }
  void ReadState( RemotePeer &peer ) {
    static_cast<SimpleRemotePeer&>(peer).ReadFloat( v );
  }
  void Tick( unsigned int time ) {
    printf( "server: %d %f\n", time, v );
  }
};

const unsigned int GAME_TICK = 16;	// one game tick every 16ms
// TODO
const unsigned int SNAPHOT_FRAMES = 5;	// one game state snapshot every 5 game frames: every 80ms

class SimpleNode : public Node {
  Object * o;
  Proxy * p;
  RemotePeer * c;
  unsigned int time;
public:
  SimpleNode() : o( nullptr ), p( nullptr ), c( nullptr ), time( 0 ) { }

  /*
   * API
   */

  void AddObject( Object * _o ) { o = _o; }
  void AddProxy( Proxy * _p ) { p = _p; }
  void AddPeer( RemotePeer * _c ) { c = _c; }
  void Tick() {
    time += GAME_TICK;
    if ( p != nullptr ) {
      p->ReadState( *c );
      p->Tick( time );
    }
    if ( o != nullptr ) {
      o->Tick( time );
      o->WriteState( *c );
    }
  }

  /*
   * Internal
   */

  unsigned int GetTime() const { return time; }
};

int main( const int argc, const char *argv[] ) {
  ScalarObject o;
  SimpleNode server;
  server.AddObject( &o );

  ScalarProxy p;  
  SimpleNode client;
  client.AddProxy( &p );

  SimpleRemotePeer c2s;
  SimpleRemotePeer s2c;
  client.AddPeer( &c2s );
  server.AddPeer( &s2c );

  while ( server.GetTime() < 30000 ) {
    s2c.DoTransport( c2s );
    server.Tick();
    client.Tick();    
  }

  return 0;
}
