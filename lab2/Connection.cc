/**
 * Connection.cc
 *
 * Created by: Linus Mellberg
 * 2013-09-06
 *
 * Class definition for Connection which handles a connection to the proxy.
 *
 */

#include <cstring>
#include <errno.h>
#include <iostream>
#include <unistd.h>

#include "Connection.h"

using namespace std;

const int BUFFLENGTH = 4096; /* Internal buffer length used in recv and send. */
const bool DEBUG = false;

Connection::Connection() : sockfd(0),
		rBuff(new char[BUFFLENGTH]), rBuffPos(0), rBuffLength(0),
		sBuff(new char[BUFFLENGTH]), connected(true){}

Connection::Connection(int socket) : sockfd(socket),
		rBuff(new char[BUFFLENGTH]), rBuffPos(0), rBuffLength(0),
		sBuff(new char[BUFFLENGTH]), connected(true){}

Connection::Connection(const Connection &old): sockfd(old.sockfd),
		rBuff(new char[BUFFLENGTH]), rBuffPos(0), rBuffLength(0), sBuff(new char[BUFFLENGTH]),
		connected(true){
	for(int i = 0; i < BUFFLENGTH; ++i){
		rBuff[i] = old.rBuff[i];
		sBuff[i] = old.sBuff[i];
	}
}

Connection& Connection::operator=(const Connection &rhs){
	if (this == &rhs) return *this;
	for(int i = 0; i < BUFFLENGTH; ++i){
		rBuff[i] = rhs.rBuff[i];
		sBuff[i] = rhs.sBuff[i];
	}
	sockfd = rhs.sockfd;
	return *this;
}

Connection::~Connection(){
	delete[] rBuff;
	delete[] sBuff;
}

void Connection::setConnection(Connection &conn){
	sockfd = conn.sockfd;
}

/**
 * Sends data over the connection.
 * Paramter:
 * string *s     Pointer to string with data to be sent.
 *
 * Note: The string will be deleted, when finished.
 */
void Connection::sendString(std::string *&data){
	if(!connected){
		throw ConnectionException(string("sendString called while not connected."),
				ConnectionException::NOT_CONNECTED);
	}
	size_t len = data->length();
	size_t pos = 0;
	size_t sent = 0;
	while(len > 0){
		sent = 0;
		if (len > BUFFLENGTH){
			memcpy(sBuff, data->c_str()+pos, BUFFLENGTH);

			while(sent < BUFFLENGTH){
				int n = 0;
				if((n = send(sockfd, sBuff+sent, BUFFLENGTH-sent, 0)) == -1){
					if(errno == EPIPE){
						connected = false;
						throw ConnectionException(string("send failed: Broken Pipe") + string(strerror(errno)),
								ConnectionException::BROKEN_PIPE);
					}
					else{
						throw ConnectionException(string("send failed") + string(strerror(errno)),
								ConnectionException::SEND_ERROR);
					}
				}
				sent += n;
			}
			if(DEBUG) cerr << ">>>>>>" << string(sBuff, sent) << "||" << sent << endl;
			pos += BUFFLENGTH;
			len -= BUFFLENGTH;
		}
		else{
			memcpy(sBuff, data->c_str()+pos, len);
			while(sent < len){
				int n = 0;
				if((n = send(sockfd, sBuff+sent, len-sent, 0)) == -1){
					if(errno == EPIPE){
						connected = false;
						throw ConnectionException(string("send failed: Broken Pipe") + string(strerror(errno)),
								ConnectionException::BROKEN_PIPE);
					}
					else{
						throw ConnectionException(string("send failed") + string(strerror(errno)),
								ConnectionException::SEND_ERROR);
					}
				}
				sent += n;
			}
			if(DEBUG) cerr << ">>>>>>" << string(sBuff, sent) << "||asdf" << sent << endl;
			len = 0;
		}
	}
	delete data;
	data = nullptr;
}

/**
 * Receives data until the terminating byte term is found or the connection is closed.
 * Parameter:
 * char term 		Terminating character.
 *
 * Return value:
 * A string pointer, the string will be terminated by term.
 *
 * Note: The returned string must be deleted.
 */
string *Connection::recvTerminatedString(char term){
	string *s = new string();
	char *hit;
	for(;;){
		if(rBuffLength == 0){
			if(connected){
				updateRBuff();
			}
			else{
				break;
			}
		}
		hit = reinterpret_cast<char*>(memchr(reinterpret_cast<void*>(getRBuff()), term, rBuffLength));
		if(hit){
			appendRBuff(s, (hit - getRBuff()) + 1);
			break;
		}
		else{
			appendRBuff(s, rBuffLength);
		}
	}
	return s;
}

/**
 * Receives len bytes of data or all data received before the connection is closed.
 * Parameter:
 * size_t len 		Length of returned string.
 *
 * Return value:
 * A string pointer, the string will be at most len characters long.
 *
 * Note: The returned string must be deleted.
 */
string *Connection::recvString(size_t len){
	size_t startlen = len;
	string *s = new string();
	for(;len > 0;){
		//cerr << s->length() << " " << rBuffPos << " " << rBuffLength << " " << len << endl;
		if(rBuffLength == 0){
			if(connected){
				updateRBuff();
			}
			else break;
		}
		//cerr << s->length() << " " << rBuffPos << " " << rBuffLength << endl;
		if(rBuffLength >= len){
			appendRBuff(s, len);
			len = 0;
		}
		else if(rBuffLength > 0){
			len = len - rBuffLength;
			appendRBuff(s, rBuffLength);
		}
	}
	if (startlen != s->length()){
		//cerr << "Not all that was requested is received!! " << startlen << " " << s->length() << endl;
	}
	return s;
}

/**
 * Is the connection good.
 */
bool Connection::isGood(){
	return connected;
}

/**
 * Appends len bytes from rBuff to the string pointed to by s.
 * If len is larger than data currently in rBuff an ConnectionException is thrown.
 */
void Connection::appendRBuff(string *s, size_t len){
	if(rBuffLength < len){
		throw ConnectionException("Read outside rBuff.", ConnectionException::READ_OUTSIDE_RBUFF);
	}
	s->append(rBuff+rBuffPos, len);
	rBuffPos += len;
	rBuffLength -= len;
}

/**
 * Receives new data to rBuff.
 * Throws ConnectionException if recv() fails.
 */
void Connection::updateRBuff(){
	if(rBuffLength > 0){
		throw ConnectionException("Update of rBuff when it's not empty.",
															ConnectionException::RBUFF_NOT_EMPTY);
	}
	int len;
	errno = 0;
	len = recv(sockfd, rBuff, BUFFLENGTH, 0);
	if(DEBUG) cerr << "<<<<<<" << string(rBuff, len) << "||" << len << endl;
	if(len == -1){
		if (errno == ECONNRESET || errno == ETIMEDOUT){
			connected = false;
		}
		else{
			throw ConnectionException(
						string("updateRBuff, recv() failed: ") +
						string(strerror(errno)),
						ConnectionException::RECEIVE_ERROR);
		}
	}
	if(len == 0){
		connected = false;
	}
	rBuffPos = 0;
	rBuffLength = len;
}

/**
 * Returns a pointer to start of not extracted data in rBuff.
 */
char *Connection::getRBuff(){
	return rBuff + rBuffPos;
}

void Connection::closeConnection(){
	//shutdown(sockfd, SHUT_RDWR);
	close(sockfd);
	//connected = false;
}
