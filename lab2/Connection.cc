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

Connection::Connection() : socket(), addr(0), rBuff(new char[BUFFLENGTH]), rBuffPos(0),
		rBuffLength(0), sBuff(new char[BUFFLENGTH]), data(){}

Connection::Connection(const Connection &old): socket(old.socket), addr(old.addr),
		rBuff(new char[BUFFLENGTH]), rBuffPos(0), rBuffLength(0), sBuff(new char[BUFFLENGTH]), data(old.data)
{
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
	data = rhs.data;
	return *this;
}

Connection::~Connection(){
	delete[] rBuff;
	delete[] sBuff;
	//if(addr) delete addr;
}

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
		}
	}
	delete data;
}

string *Connection::recvString(char term){return NULL;}

string *Connection::recvString(int len){
	string *s = new string();
	for(;;){
		if(rBuffLength > len){
			appendRBuff(s, len);
			len = 0;
		}
		else if(rBuffLength > 0){
			appendRBuff(s, rBuffLength);
			len -= rBuffLength;
		}
		if(len == 0) break;

		if(rBuffLength == 0){
			updateRBuff();
		}
	}
	return s;
}

void Connection::appendRBuff(string *s, int len){
	if(rBuffLength > len) throw ConnectionException("Read outside rBuff.");
	s->append(rBuff, rBuffPos, len);
	rBuffPos += len;
	rBuffLength -= len;
}

void Connection::updateRBuff(){
	if(rBuffLength > 0) throw ConnectionException("Update of rBuff when it's not empty.");
	int len;
	len = recv(socket, rBuff, BUFFLENGTH, 0);
	if(len == -1) throw ConnectionException(string("recv() failed: ") + string(strerror(errno)));
	rBuffPos = 0;
	rBuffLength = len;
}
