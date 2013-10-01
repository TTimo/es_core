/*
 * Abstract API for the networking functionality in es_core
 */

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

