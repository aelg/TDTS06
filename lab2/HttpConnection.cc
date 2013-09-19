/*
 * HttpConnection.cpp
 *
 *  Created by: Linus Mellberg
 *  7 sep 2013
 */

#include <sstream>
#include <algorithm>

#include "HttpConnection.h"
#include <iostream>

using namespace std;

HttpConnection::HttpConnection(const Connection &conn) : Connection(conn), rStatusLine(0),
		rHeader(), rData(0), sStatusLine(0), sHeader(), sData(0), statusCode(0){}

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
void HttpConnection::clearHeaderField(){
	while(!sHeader.empty()){
		delete sHeader.front();
		sHeader.pop();
	}
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
	if(sData){
		delete sData;
	}
	sData = s;
	s = nullptr;
}
void HttpConnection::addData(std::string *&&s){
	if(sData){
		delete sData;
	}
	sData = s;
}
void HttpConnection::sendData(){
	sendString(sData);
	sData = nullptr;
}

HeaderField *HttpConnection::getHeaderField(){
	if(rHeader.empty()){
		return nullptr;
	}
	HeaderField *h = rHeader.front();
	rHeader.pop();
	return h;
}
std::string* HttpConnection::getStatusLine(){
	string *t;
	t = rStatusLine;
	rStatusLine = nullptr;
	return t;
}
string* HttpConnection::getData(){
	string *t;
	t = rData;
	rData = nullptr;
	return t;
}
HttpConnection::ReceivedType HttpConnection::recvStatusLine(){
	rStatusLine = recvTerminatedString('\n');
	statusCode = 0;
	while(rStatusLine->back() == '\r' || rStatusLine->back() == '\n'){
		rStatusLine->pop_back();
	}
	stringstream statusLine(*rStatusLine);
	string s;
	statusLine >> s;
	if(s == "HTTP/1.1" || s == "HTTP/1.0"){ // This is a response
		statusLine >> statusCode;
		cerr << *rStatusLine << endl;
		return RESPONSE;
	}
	else if(s == "GET"){ // GET request all is well.
		return GET_REQUEST;
	}
	else if(s == "POST"){ // POST request all is well.
		return POST_REQUEST;
	}
	else if(s == "OPTIONS" ||
					s == "HEAD" ||
					//s == "POST" ||
					s == "PUT" ||
					s == "DELETE" ||
					s == "TRACE" ||
					s == "CONNECT"){ /* Valid but we cannot handle them drop the connection */
		cerr << "Unimplemented request: " << s << endl;
		return NOT_IMPLEMENTED_REQUEST;
	}
	else{
		throw HttpConnectionException(string("Not a valid HTTP GET request: ") + *rStatusLine);
	}
}


void HttpConnection::recvHeader(){
	string *headerLine = nullptr;
	for (;;){
		headerLine = recvTerminatedString('\n');
		if (*headerLine == "\r\n")
			break;
		size_t posColon = headerLine->find_first_of(':');
		size_t posValue = headerLine->find_first_not_of(": ", posColon);
		size_t posNewLine = headerLine->find_first_of("\n\r", posColon);
		if (posColon == string::npos || posValue == string::npos || posNewLine == string::npos )
			throw HttpConnectionException("Empty string or string not containing header");
		HeaderField *currentHeader = new HeaderField(
				string(*headerLine, 0, posColon),
				string(*headerLine,posValue, posNewLine - posValue));
		rHeader.push(currentHeader);
		delete headerLine;
		headerLine = nullptr;
	}
	if(headerLine) delete headerLine;
}

void HttpConnection::recvData(size_t length){
	if(isGood()){
		rData = recvString(length);
	}
}

/**
 * Receives one chunk of a chunked transfer.
 * Returns false when the received chunk was the last.
 */
bool HttpConnection::recvChunk(size_t length){
	size_t chunkLength = 0;
	bool moreChunks = true;
	static size_t leftOfChunk = 0;
	string *size;
	string *chunk;
	rData = new string();
	if(!leftOfChunk){
		stringstream ss;
		size = recvTerminatedString('\n');
		ss >> *size;
		rData->append(*size);
		ss >> hex >> chunkLength;
		leftOfChunk = chunkLength;
		if(!ss.good()) cerr << "Warning chunklength extraction failed." << endl;
	}
	if(leftOfChunk > 0){
		int recvSize = min(leftOfChunk+2, length);
		chunk = recvString(recvSize);
		leftOfChunk -= recvSize;
	}
	else{
		moreChunks = false;
		chunk = new string();
		string *trailer = recvTerminatedString('\n');
		while(*trailer != "\r\n"){
			chunk->append(*trailer);
			delete trailer;
			trailer = recvTerminatedString('\n');
			if(trailer->length() == 0) break;
		}
		chunk->append(*trailer);
		chunk->append("\r\n");
		delete trailer;
	}
	delete size;
	rData->append(*chunk);
	delete chunk;

	return moreChunks;
}

int HttpConnection::getStatusCode(){
	return statusCode;
}

void HttpConnection::closeConnection(){
	Connection::closeConnection();
}


