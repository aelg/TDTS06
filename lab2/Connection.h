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

#include <netdb.h>
#include <string>
#include <stdexcept>

class ConnectionException : public std::logic_error {
 public:
  explicit ConnectionException(const std::string& what)throw() :logic_error(what) {};
};

class Connection{
public:
	Connection(int socket, addrinfo *addr);
	Connection(const Connection &old);
	Connection& operator=(const Connection &rhs);
	~Connection();

	void sendString(const std::string *data);
	std::string *recvTerminatedString(char term);
	std::string *recvString(size_t len);

private:
	void appendRBuff(std::string *s, size_t len);
	void updateRBuff();
	char *getRBuff();

	int socket;
	addrinfo *addr;
	char *rBuff;
	size_t rBuffPos;
	size_t rBuffLength;
	char *sBuff;
};

#endif /* CONNECTION_H_ */
