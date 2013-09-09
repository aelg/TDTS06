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

class Connection{
public:
	Connection();
	Connection(const Connection &old);
	Connection& operator=(const Connection &rhs);
	~Connection();

	void send(const std::string *data);
	std::string *recv(char term);
	std::string *recv(int len);

	int socket;
	addrinfo *addr;
	char *rBuff;
	char *sBuff;
	std::string data;
};

#endif /* CONNECTION_H_ */
