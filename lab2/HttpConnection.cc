/*
 * HttpConnection.cpp
 *
 *  Created by: Linus Mellberg
 *  7 sep 2013
 */

#include <sstream>

#include "HttpConnection.h"

using namespace std;

HttpConnection::HttpConnection(const Connection &conn) : Connection(conn), rStatusLine(0),
	rHeader(), rData(0), sStatusLine(0), sHeader(), sData(0){}

HttpConnection::~HttpConnection() {
	if(rStatusLine) delete rStatusLine;
	if(sStatusLine) delete sStatusLine;
	if(rData) delete rData;
	if(sData) delete sData;
	while(!rHeader.empty()){
		delete rHeader.front();
		rHeader.pop();
	}
	while(!sHeader.empty()){
		delete sHeader.front();
		sHeader.pop();
	}
}

void HttpConnection::setConnection(Connection &conn){
	Connection::setConnection(conn);
}

HeaderField *HttpConnection::newHeaderField(const string &name, const string &value){
	return new HeaderField(string(name), string(value));
}

void HttpConnection::setStatusLine(std::string *&s){
	sStatusLine = s;
	s = nullptr;
}
void HttpConnection::setStatusLine(std::string *&&s){
	sStatusLine = s;
}
void HttpConnection::sendStatusLine(){
	sStatusLine->append("\r\n");
	sendString(sStatusLine);
	sStatusLine = nullptr;
}
void HttpConnection::addHeaderField(HeaderField *&header){
	sHeader.push(header);
	header = nullptr;
}
void HttpConnection::addHeaderField(const string &name, const string &value){
	sHeader.push(newHeaderField(name, value));
}
void HttpConnection::addContentLength(){
	stringstream ss;
	if(!sData) throw HttpConnectionException("addContentLength called with no added data.");
	ss << sData->length();
	addHeaderField("Content-Length", string(ss.str()));
}
void HttpConnection::sendHeader(){
	string *s = new string();
	while(!sHeader.empty()){
		s->append(sHeader.front()->first);
		s->append(": ");
		s->append(sHeader.front()->second);
		s->append("\r\n");
		delete sHeader.front();
		sHeader.pop();
	}
	s->append("\r\n");
	sendString(s);
}
void HttpConnection::addData(std::string *&s){
	sData = s;
	s = nullptr;
}
void HttpConnection::addData(std::string *&&s){
	sData = s;
}
void HttpConnection::sendData(){
	sendString(sData);
	sData = nullptr;
}

HeaderField *HttpConnection::getHeaderField(){return nullptr;}
std::string *HttpConnection::getStatusLine(){return nullptr;}
std::string *HttpConnection::getData(){return nullptr;}
