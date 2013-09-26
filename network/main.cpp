/*
 * API
 */

#include <stdio.h>
#include <assert.h>
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
  virtual void ReadState( unsigned int t, RemotePeer &peer ) = 0;
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
  virtual void WriteState() = 0;
};

/*
 * Demo implementation
 */

// store a single message at a time, can only be written and read back out once (with internal state to make sure)
class SimpleRemotePeer : public RemotePeer {
  float v;
  bool read_ready;
public:
  SimpleRemotePeer() : v(0.0f), read_ready(false) { }

  /*
   * Internal
   */

  void DoTransport( SimpleRemotePeer &remote ) {
    // mimics what happens if two peers actually talk to each other (across threads, TCP, UDP etc.)
    remote.WriteFloat( v );
    read_ready = false;
  }
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
    printf( "server: %d %f\n", time, v );
  }
};

class ScalarProxyState {
public:
  float v;
  unsigned int t;
  ScalarProxyState() : v( 0.0f ), t( 0 ) { }
};

class ScalarProxy : public Proxy {
  ScalarProxyState s1, s2;
  unsigned int state_counter;	// can only start interpolating once enough data has come in
public:
  ScalarProxy() : state_counter( 0 ) { }

  /*
   * Internal
   */

  bool CanInterpolate() {
    return state_counter >= 2;
  }

  /*
   * API
   */

  void ReadState( unsigned int t, RemotePeer &peer ) {
    float _v;
    if ( !static_cast<SimpleRemotePeer&>(peer).ReadFloat( _v ) ) {
      return;
    }
    // new state received
    s1 = s2;
    s2.v = _v;
    s2.t = t;
    state_counter += 1;
  }
  void Tick( unsigned int time ) {
    if ( !CanInterpolate() ) {
      printf( "client: no interpolation yet\n" );
      return;
    }
    float t1 = (float)s1.t;
    float t2 = (float)s2.t;
    // NOTE: we use t - t2, assuming all snapshots come at the same interval
    float ratio = ( (float)time - t2 ) / ( t2 - t1 );
    if ( ratio > 1.0f ) {
      printf( "client: block extrapolate %f\n", ratio );
      ratio = 1.0f;
    }
    float v = ratio * ( (float)s2.v - (float)s1.v ) + (float)s1.v;
    printf( "client: time %d interpolate ratio %f value %f\n", time, ratio, v );
  }
};

const unsigned int GAME_TICK = 16;	// one game tick every 16ms
const unsigned int SNAPSHOT_FRAMES = 5;	// one game state snapshot every 5 game frames: every 80ms

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
      p->ReadState( time, *c );
      p->Tick( time );
    }
    if ( o != nullptr ) {
      o->Tick( time );
    }
  }
  void WriteState() {
    if ( o == nullptr ) {
      return;
    }
    o->WriteState( *c );
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
