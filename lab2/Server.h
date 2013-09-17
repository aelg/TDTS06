/*
 * Server.h
 *
 * Created by: Linus Mellberg
 * 2011-09-05
 *
 * Server class that handles incoming connection and forks of proxy instances.
 */

#ifndef SERVER_H
#define SERVER_H
#include <stdexcept>
#include <string>
#include <netdb.h>

#include "Connection.h"

extern const char* DEFAULT_PORT;

class ServerException : public std::logic_error {
 public:
	enum Error{
		ACCEPT_FAILED = 1,
		ACCEPT_INTERRUPTED,
		GETADDRINFO_FAILED,
		BIND_FAILED,
		LISTEN_FAILED
	};
  explicit ServerException(const std::string& what, Error errorNumber)throw():
  	logic_error(what), errorNumber(errorNumber){};
  int errorNumber;

};

class Server {
  public:
    Server(const char* port = DEFAULT_PORT);
    ~Server();
    Connection *acceptNew();
    void stopListening();
    void setPort(const char* port);

    static const int BACKLOG = 4;
    static const int ERROR = -1;
    static const int DATALENGTH = 1000;

  protected:
  private:

    int s;
    const char* port;
};

#endif
