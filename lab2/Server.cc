/**
 * Server.cc
 *
 * Created by: Linus Mellberg
 * 2013-09-05
 *
 * Class definition of Server class that handles incoming connections.
 *
 */

#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>

#include "Server.h"

using namespace std;

const char* DEFAULT_PORT = "8080";
const int STOP_RECEIVING = 0;

/**
 * Constructor
 *
 * Parameters:
 * port: Port number to listen to.
 *
 */
Server::Server(const char* port) : port(port){
  int status;
  int yes = 1;
  struct addrinfo hints;
  struct addrinfo *servinfo;
  memset(&hints, 0, sizeof(hints));

  // IPv4
  hints.ai_family = AF_INET;

  // TCP
  hints.ai_socktype = SOCK_STREAM;

  // Listen to default address.
  hints.ai_flags = AI_PASSIVE;

  // Get addrinfo
  if((status = getaddrinfo(NULL, port, &hints, &servinfo)) != 0){
    throw(ServerException(string("getaddrinfo failed") + string(gai_strerror(status)),
    											ServerException::GETADDRINFO_FAILED));
  }

  // Find a working socket.
  struct addrinfo *p;
  for(p = servinfo; p != 0; p = p->ai_next){
    if((s = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
      cerr << "socket failed, trying next addrinfo" << endl;
      continue;
    }

    // Set reuseraddr.
    if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
      cerr  << "setsockopt failed, trying next addrinfo" << endl;
      continue;
    }

    // Bind socket.
    if(bind(s, p->ai_addr, p->ai_addrlen) == -1){
      cerr  << string("bind failed: \"") +
      		     string(strerror(errno)) +
      		     string("\" trying next addrinfo") << endl;
      break;
    }
    break;
  }

  //Not needed any more.
  freeaddrinfo(servinfo);

  // Test if socket was found.
  if (p == 0)
    throw(ServerException("failed to bind!", ServerException::BIND_FAILED));

  // Listen to socket.
  if (listen(s, BACKLOG) == -1)
    throw(ServerException("listen failed", ServerException::LISTEN_FAILED));

  // Server is ready to accept connections.
  cout << "Waiting for connections." << endl;
}

Server::~Server(){
  shutdown(s, SHUT_RDWR);
}

Connection *Server::acceptNew(){
  
  struct sockaddr their_addr;
  socklen_t addr_size;
  int newSocket = 0;

  //their_addr = new sockaddr;
  addr_size = sizeof their_addr;
  if((newSocket = accept(s, &their_addr, &addr_size)) == -1){
  	if(errno == EINTR){
  		throw(ServerException(string("accept failed: ") + string(strerror(errno)),
  		  										ServerException::ACCEPT_INTERRUPTED));
  	}
  	else{
  		throw(ServerException(string("accept failed: ") + string(strerror(errno)),
  												  ServerException::ACCEPT_FAILED));
  	}
  }

  //cout << "New connection." << endl;

  return new Connection(newSocket);

}

void Server::stopListening(){
	shutdown(s, SHUT_RDWR);
}
