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
    throw(ServerException(string("getaddrinfo failed") + string(gai_strerror(status))));
  }

  // Find a working socket.
  struct addrinfo *p;
  for(p = servinfo; p != 0; p = p->ai_next){
    if((s = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
      cerr << "socket failed, trying next addrinfo" << endl;
      continue;
    }

    /*if(fcntl(s, F_SETFL, O_NONBLOCK) == -1){
      shutdown(s, STOP_RECEIVING);
      cerr << "setting nonblock failed, trying next addrinfo" << endl;
      continue;
    }*/

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

  //Not needed.
  freeaddrinfo(servinfo);

  // Test socket.
  if (p == 0)
    throw(ServerException("failed to bind!"));

  // Listen to socket.
  if (listen(s, BACKLOG) == -1)
    throw(ServerException("listen failed"));

  // Server is ready to accept connections.
  cout << "Waiting for connections." << endl;
}

Server::~Server(){
  shutdown(s, STOP_RECEIVING);
}

Connection *Server::acceptNew(){
  
  struct sockaddr_storage *their_addr;
  socklen_t addr_size;

  int newSocket = 0;

  Connection *newConnection = new Connection;


  their_addr = new sockaddr_storage;
  addr_size = sizeof their_addr;
  if((newSocket = accept(s, (sockaddr*) their_addr, &addr_size)) == -1){
  	if (errno == EWOULDBLOCK || errno == EAGAIN){
  		delete their_addr;
  		cout << "No new connections" << endl;
  		return NULL;
  	}
  	else throw(ServerException(string("accept failed") + string(gai_strerror(newSocket))));
  }

  cout << "New connection." << endl;
  if(fcntl(newSocket, F_SETFL, O_NONBLOCK) == -1){
  	shutdown(newSocket, STOP_RECEIVING);
  	delete their_addr;
  	cerr << "setting nonblock failed, connection closed" << endl;
  	return NULL;
  }
  newConnection->socket = newSocket;
  newConnection->addr = (addrinfo*) their_addr;

  return newConnection;

}

/*
  for(vector<Connection>::iterator it = newlyAccepted.begin(); it != newlyAccepted.end(); ++it){

    int res;
    if((res = recv(it->socket, it->buff, DATALENGTH, 0)) != -1){
      if(errno != EWOULDBLOCK && errno != EAGAIN){
        // Bad socket remove.
    	shutdown(it->socket, STOP_RECEIVING);
        it = newlyAccepted.erase(it);
        --it;
      }
      else { // Har skickat data
        it->data.append(it->buff, 0, res);
        size_t pos;
        bool login;
        if(it->data[0] == 'n' || it->data[0] == 'l'){
          if(it->data[0] == 'n') login = false;
          else login = true;
          it->data.erase(0,1);
        }
        else{
          it->data.erase();
          return;
        }
        if((pos = it->data.find(" #", it->data.find_first_of(' ')+1)) != string::npos){ // Korrekt autentiseringsdata.
          cout << "Autentiserar: " << string(it->data, 0, it->data.find_first_of(' ')) << endl;
          // Fr�ga UserRegister om vi har f�tt r�tt uppgifter
          string user(it->data, 0, it->data.find_first_of(' '));
          it->data.erase(0, it->data.find_first_of(' ') + 1);
          string pass(it->data, 0, it->data.find(" #"));
          it->data.erase(0, it->data.find(" #") + 3);

          AuthMessage auth(user, pass, ++idCount);
          if(login)
            sendMessage("UserRegister", AUTH,  auth.serialize());
          else
            sendMessage("UserRegister", CREATEUSER,  auth.serialize());

          waitingForAuthentication.insert(make_pair(idCount, *it));
          it = newlyAccepted.erase(it);
          --it;
        }
        else {
          size_t pos = it->data.find_last_of('\n');
          if(pos == string::npos)
            it->data.erase();
          else
            it->data.erase(0, it->data.find_last_of('\n')+1);
        }
      }
    }
  }

  cout << newConnection.data;
  return;
}
*/
