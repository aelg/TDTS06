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

Connection::Connection() : socket(), addr(0), rBuff(new char[BUFFLENGTH]),
		sBuff(new char[BUFFLENGTH]), data(){}

Connection::Connection(const Connection &old): socket(old.socket), addr(old.addr),
		rBuff(new char[BUFFLENGTH]), sBuff(new char[BUFFLENGTH]), data(old.data)
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
