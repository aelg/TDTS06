/*
 * HttpConnection.cpp
 *
 *  Created by: Linus Mellberg
 *  7 sep 2013
 */

#include <sstream>

#include "HttpConnection.h"
#include <iostream>

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

HeaderField *HttpConnection::newHeaderField(const std::string &name, const std::string &value){
	return new HeaderField(name, value);
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
void HttpConnection::addHeaderField(const std::string &name, const std::string &value){
	sHeader.push(newHeaderField(name, value));
}
void HttpConnection::addContentLength(){
	stringstream ss;
	if(!sData) throw HttpConnectionException("addContentLength called with no added data.");
	ss << sData->length();
	addHeaderField("Content-Length", ss.str());
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
std::string* HttpConnection::getStatusLine(){
	return rStatusLine;
}
string* HttpConnection::getData(){
	return rData;
}
void HttpConnection::recvStatusLine(){
	rStatusLine = recvTerminatedString('\n');
	if(rStatusLine->find("GET")== string::npos){
		delete rStatusLine;
		rStatusLine = new string("");
		throw HttpConnectionException("Not a valid HTTP GET request");
	}
}


void HttpConnection::recvHeader(){
	string *headerLine;
	for (;;){
		headerLine = recvTerminatedString('\n');
		if (*headerLine == "\r\n")
			break;
		size_t posColon = headerLine->find_first_of(':', 0);
		size_t posFirstChar = headerLine->find_first_not_of(' ');
		size_t posNewLine = headerLine->find_first_of('\r');
		if (posColon == string::npos || posFirstChar == string::npos || posNewLine == string::npos )
			throw HttpConnectionException("Empty string or string not containing header");
		HeaderField *currentHeader = new HeaderField(
				string(*headerLine,posFirstChar, posColon-posFirstChar),
				string(*headerLine,posColon + 1, posNewLine - posColon));
		rHeader.push(currentHeader);
		delete headerLine;
	}
}

void HttpConnection::recvData(){
	rData = recvTerminatedString('\n');
}


