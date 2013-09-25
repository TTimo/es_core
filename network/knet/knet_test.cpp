#include "kNet.h"

static const unsigned short LISTEN_PORT = 26000;

class MessageListener : public kNet::IMessageHandler {
public:
  void HandleMessage( kNet::MessageConnection *source, kNet::packet_id_t packet_id, kNet::message_id_t message_id, const char *data, size_t numBytes ) {
    kNet::DataDeserializer dd( data, numBytes );
    printf( "received message: %s\n", dd.ReadString().c_str() );
  }
};

class ServerListener : public kNet::INetworkServerListener {
public:
  bool NewConnectionAttempt(
			    const kNet::EndPoint &endPoint,
			    const char *data,
			    size_t numBytes ) {
    printf( "Connection attempt from %s\n", endPoint.ToString().c_str() );
    return true;
  }

  void ClientDisconnected( kNet::MessageConnection *connection ) {
    printf( "Client disconnected: %s\n", connection->RemoteEndPoint().ToString().c_str() );    
  }			  

  void NewConnectionEstablished( kNet::MessageConnection *connection ) {
    printf( "Connection established from %s\n", connection->RemoteEndPoint().ToString().c_str() );
    MessageListener *l = new MessageListener();
    connection->RegisterInboundMessageHandler( l );
    kNet::NetworkMessage *msg = connection->StartNewMessage( 0, 256 );
    msg->reliable = true;
    kNet::DataSerializer ds( msg->data, 256 );
    ds.AddString( connection->RemoteEndPoint().ToString() );
    connection->EndAndQueueMessage( msg );
  }
};

int main( int argc, char *argv[] ) {
  kNet::Network network;

  if ( argc >= 2 ) {
    MessageListener client_listener;
    Ptr(kNet::MessageConnection) connection = network.Connect( argv[1], LISTEN_PORT, kNet::SocketOverUDP, &client_listener );
    printf( "Connecting to remote server at %s\n", argv[1] );
    connection->RunModalClient();
  } else {
    ServerListener listener;
    kNet::NetworkServer * server = network.StartServer( LISTEN_PORT, kNet::SocketOverUDP, &listener, false );
    std::vector< kNet::Socket * >::iterator isocket, end;
    end = server->ListenSockets().end();
    for ( isocket = server->ListenSockets().begin(); isocket != end; ++isocket ) {
      printf( "Local socket: %s\n", (*isocket)->ToString().c_str() );
    }
    printf( "Running in server mode. Waiting for connections\n" );
    server->RunModalServer();
  }
  return 0;
}
