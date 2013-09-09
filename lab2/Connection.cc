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

#include "Connection.h"

using namespace std;

const int BUFFLENGTH = 1000;
const int STOP_RECEIVING = 0;


Connection::Connection(int socket, addrinfo *addr) : socket(socket), addr(addr),
		rBuff(new char[BUFFLENGTH]), rBuffPos(0), rBuffLength(0), sBuff(new char[BUFFLENGTH]){}

Connection::Connection(const Connection &old): socket(old.socket), addr(old.addr),
		rBuff(new char[BUFFLENGTH]), rBuffPos(0), rBuffLength(0), sBuff(new char[BUFFLENGTH]){
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
	socket = rhs.socket;
	addr = rhs.addr;
	return *this;
}

Connection::~Connection(){
	delete[] rBuff;
	delete[] sBuff;
	shutdown(socket, STOP_RECEIVING);
	//if(addr) delete addr;
}

/**
 * Sends data over the connection.
 * Paramter:
 * string *s     Pointer to string with data to be sent.
 *
 * Note: The string will be deleted, when finished.
 */
void Connection::sendString(const std::string *data){
	int len = data->length();
	int pos = 0;
	while(len > 0){
		if (len > BUFFLENGTH){
			memcpy(sBuff, data->c_str()+pos, BUFFLENGTH);
			send(socket, sBuff, BUFFLENGTH, 0);
			pos += BUFFLENGTH;
			len -= BUFFLENGTH;
		}
		else{
			memcpy(sBuff, data->c_str()+pos, len);
			send(socket, sBuff, len, 0);
			len = 0;
		}
	}
	delete data;
}

/**
 * Receives data until the terminating byte term is found.
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
			updateRBuff();
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
 * Receives len bytes of data.
 * Parameter:
 * size_t len 		Length of returned string.
 *
 * Return value:
 * A string pointer, the string will be len characters long.
 *
 * Note: The returned string must be deleted.
 */
string *Connection::recvString(size_t len){
	string *s = new string();
	for(;len > 0;){
		if(rBuffLength == 0){
			updateRBuff();
		}
		if(rBuffLength >= len){
			appendRBuff(s, len);
			len = 0;
		}
		else if(rBuffLength > 0){
			appendRBuff(s, rBuffLength);
			len -= rBuffLength;
		}
		if(len == 0) break;
	}
	return s;
}

/**
 * Appends len bytes from rBuff to the string pointed to by s.
 * If len is larger than data currently in rBuff an ConnectionException is thrown.
 */
void Connection::appendRBuff(string *s, size_t len){
	if(rBuffLength < len){
		throw ConnectionException("Read outside rBuff.");
	}
	s->append(rBuff, rBuffPos, len);
	rBuffPos += len;
	rBuffLength -= len;
}

/**
 * Receives new data to rBuff.
 * Throws ConnectionException if recv() fails.
 */
void Connection::updateRBuff(){
	if(rBuffLength > 0){
		throw ConnectionException("Update of rBuff when it's not empty.");
	}
	ssize_t len;
	errno = 0;
	len = recv(socket, rBuff, BUFFLENGTH, 0);
	if(len == -1){
		throw ConnectionException(string("updateRBuff, recv() failed: ") + string(strerror(errno)));
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
