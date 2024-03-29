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

extern int const BUFFLENGTH;

class ConnectionException : public std::logic_error {
 public:
	enum Error{
		NOT_CONNECTED = 1,
		RECEIVE_ERROR,
		READ_OUTSIDE_RBUFF,
		RBUFF_NOT_EMPTY,
		SEND_ERROR,
		BROKEN_PIPE
	};
  explicit ConnectionException(const std::string& what, Error error) throw() :
  	logic_error(what), error_number(error) {};

  Error error_number;
};

class Connection{
public:
	Connection();
	Connection(int socket);
	Connection(const Connection &old);
	Connection& operator=(const Connection &rhs);
	~Connection();
	void setConnection(Connection &conn);

	void sendString(std::string *&data);
	std::string *recvTerminatedString(char term);
	std::string *recvString(size_t len);

	bool isGood();

	void closeConnection();

private:
	void appendRBuff(std::string *s, size_t len);
	void updateRBuff();
	char *getRBuff();

	int sockfd;
	char *rBuff;
	size_t rBuffPos;
	size_t rBuffLength;
	char *sBuff;
	bool connected;
};

#endif /* CONNECTION_H_ */
