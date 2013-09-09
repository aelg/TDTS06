/**
 * Connection.h
 *
 * Created by: Linus Mellberg
 * 2013-09-06
 *
 * Class header for Connection which handles a connection to the proxy.
 *
 */

#ifndef CONNECTION_H_
#define CONNECTION_H_

#define BUFFLENGTH 1024

#include <netdb.h>
#include <string>
#include <stdexcept>

class ConnectionException : public std::logic_error {
 public:
  explicit ConnectionException(const std::string& what)throw() :logic_error(what) {};
};

class Connection{
public:
	Connection();
	Connection(const Connection &old);
	Connection& operator=(const Connection &rhs);
	~Connection();

	void sendString(const std::string *data);
	std::string *recvString(char term);
	std::string *recvString(int len);

//private:
	void appendRBuff(std::string *s, int len);
	void updateRBuff();

	int socket;
	addrinfo *addr;
	char *rBuff;
	int rBuffPos;
	int rBuffLength;
	char *sBuff;
	std::string data;
};

#endif /* CONNECTION_H_ */
