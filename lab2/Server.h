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
#include <map>
#include <netdb.h>

#include "Connection.h"

extern const char* DEFAULT_PORT;
extern const int STOP_RECEIVING;

class ServerException : public std::logic_error {
 public:
  explicit ServerException(const std::string& what)throw() :logic_error(what) {};
};

class Server {
  public:
    Server(const char* port = DEFAULT_PORT);
    ~Server();
    void acceptNew();

    static const int BACKLOG = 4;
    static const int ERROR = -1;
    static const int DATALENGTH = 1000;

  protected:
  private:

    int s;
    const char* port;
    std::map<pthread_t, Connection *> newlyAccepted;
};

#endif
