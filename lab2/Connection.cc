/**
 * Connection.cc
 *
 * Created by: Linus Mellberg
 * 2013-09-06
 *
 * Class definition for Connection which handles a connection to the proxy.
 *
 */

#include "Connection.h"

Connection::Connection() : socket(), addr(0), buff(new char[BUFFLENGTH]), data(){}

Connection::Connection(const Connection &old): socket(old.socket), addr(old.addr),
		buff(new char[BUFFLENGTH]), data(old.data)
{
	for(int i = 0; i < BUFFLENGTH; ++i){
		buff[i] = old.buff[i];
	}
}

Connection& Connection::operator=(const Connection &rhs){
	if (this == &rhs) return *this;
	for(int i = 0; i < BUFFLENGTH; ++i){
		buff[i] = rhs.buff[i];
	}
	socket = rhs.socket;
	addr = rhs.addr;
	data = rhs.data;
	return *this;
}

Connection::~Connection(){
	delete[] buff;
	//if(addr) delete addr;
}
