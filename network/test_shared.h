/*
 * Some shared functionality for test1/test2
 */

/*
 * store a single message at a time, can only be written and read back out once (with internal state to make sure)
 * abstracted as a pure virtual to support test1 (in process) and test2 (over tcp)
 */
class SimpleRemotePeer : public RemotePeer {
public:
  virtual void WriteFloat( float &_v ) = 0;
  virtual bool ReadFloat( float &_v ) = 0;
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
    printf( "ReadState\n" );
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
    // NOTE: we use the interval between the last two snapshots, assuming they all come at the same interval
    // it's also the time at which we *received* them, not necessarily the intended tick time on the remote peer
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
    printf( "WriteState\n" );
    o->WriteState( *c );
  }

  /*
   * Internal
   */

  unsigned int GetTime() const { return time; }
};
