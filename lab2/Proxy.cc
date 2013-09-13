/*
 * Proxy.cc
 *
 *  Created by: Linus Mellberg
 *  12 sep 2013
 */

#include <string>
#include <cstring>
#include <sstream>
#include <iostream>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "Proxy.h"

int const STOP_RECEIVING = 0;

using namespace std;

Proxy::Proxy(HttpConnection *browser) : browser(browser), server(nullptr),
																				contentLength(0), transferData(true),
																				chunkedTransfer(false){}

Proxy::~Proxy() {
	if(browser) delete browser;
	if(server) delete server;
}

void Proxy::run(){
	for(bool b = true; b; b = false){
		if(!readBrowserRequest()) break;
		if(!transferBrowserRequest()) break;
		if(!setupServerConnection()) break;
		if(!sendBrowserRequest()) break;
		if(!readServerResponse()) break;
		if(!transferServerResponseHeader()) break;
		if(!sendServerResponseHeader()) break;
		while(transferData && transferServerResponseData()){
			sendServerResponseData();
		}
	}
	closeServerConnection();
}

bool Proxy::readBrowserRequest(){
	HttpConnection::ReceivedType r;
	try{
		r = browser->recvStatusLine();
	}catch(HttpConnectionException &e){
		throw ProxyException(string("readBrowserRequest catched: ") + e.what(),
												 ProxyException::UNKNOWN_ERROR);
	}
	browser->recvHeader();
	switch(r){
	case HttpConnection::GET_REQUEST:
		break;
	case HttpConnection::RESPONSE:
		throw ProxyException("Received response from browser.", ProxyException::BAD_BROWSER);
		break;
	case HttpConnection::NOT_IMPLEMENTED_REQUEST:
		send501NotImplented();
		return false;
	default:
		ProxyException("This can't happen.", ProxyException::UNKNOWN_ERROR);
		break;
	}
	return true;
}

bool Proxy::transferBrowserRequest(){
	string* statusLine;
	HeaderField *h;
	size_t httpPos;
	serverHostname = "";

	server = new HttpConnection(Connection());

	statusLine = browser->getStatusLine();
	cerr << "Status Line: " << *statusLine << endl;

	if((httpPos = statusLine->find("http://")) != string::npos){
		statusLine->erase(httpPos, statusLine->find_first_of('/', httpPos+7) - httpPos);
	}
	server->setStatusLine(statusLine);

	while((h = browser->getHeaderField())){
		h = filterHeaderFieldOut(h);
		if(h)server->addHeaderField(h);
	}
	return true;
}

bool Proxy::setupServerConnection(){
	if(serverHostname.empty()){
		throw ProxyException("setupServerConnection: Host header not found.",
				ProxyException::HOST_HEADER_NOT_FOUND);
	}
	int sockfd, rv;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr addr;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	cerr << "Connecting to: '" << serverHostname.c_str() << '\''<< endl;
	if((rv = getaddrinfo(serverHostname.c_str(), "80", &hints, &servinfo)) != 0){
		throw ProxyException(string("Remote server setup failed, getaddrinfo: ") + string(gai_strerror(rv)),
				ProxyException::SETUP_SERVER_CONNECTION_ERROR);
	}

	for(p = servinfo; p != nullptr; p = p->ai_next){
		if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
			cerr << "Proxy::setupServerConnection: socket failed trying next addrinfo. Reason: "
					 << strerror(errno);
			continue;
		}
		if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1){
			shutdown(sockfd, STOP_RECEIVING);
			cerr << "Proxy::setupServerConnection: connect failed trying next addrinfo. Reason: "
					 << strerror(errno);
			continue;
		}
		break;
	}
	if(p == nullptr){
		throw ProxyException(string("Remote server setup failed, failed to connect. Last error: ") + strerror(errno),
						ProxyException::SETUP_SERVER_CONNECTION_ERROR);
	}
	freeaddrinfo(servinfo);
	addr = *reinterpret_cast<sockaddr*>(p);
	Connection newConnection(sockfd, addr);
	server->setConnection(newConnection);
	return true;
}
bool Proxy::sendBrowserRequest(){
	server->sendStatusLine();
	server->sendHeader();
	return true;
}
bool Proxy::readServerResponse(){
	HttpConnection::ReceivedType r;
	r = server->recvStatusLine();
	server->recvHeader();
	switch(r){
		case HttpConnection::GET_REQUEST:
			throw ProxyException("Received GET request from server.", ProxyException::BAD_SERVER);
			break;
		case HttpConnection::RESPONSE:
			if(server->getStatusCode() != 200){ //Don't know if data will be sent with other codes.
				transferData = false;
			}
			else{
				transferData = true;
			}
			break;
		case HttpConnection::NOT_IMPLEMENTED_REQUEST:
			// Shouldn't happen.
			throw ProxyException("Received unimplemented request from server.", ProxyException::BAD_SERVER);
			return true;
		default:
			ProxyException("This can't happen.", ProxyException::UNKNOWN_ERROR);
			break;
		}
	return true;
}
bool Proxy::transferServerResponseHeader(){
	cerr << "transferServerResponseHeader" << endl;
	string *statusLine;
	HeaderField *h;
	contentLength = 0;
	chunkedTransfer = false;

	statusLine = server->getStatusLine();
	browser->setStatusLine(statusLine);

	while((h = server->getHeaderField())){
		h = filterHeaderFieldIn(h);
		if(h)browser->addHeaderField(h);
	}
	return true;
}

bool Proxy::sendServerResponseHeader(){
	cerr << "sendServerResponseHeader()" << endl;
	cerr << "transferData: " << transferData << endl;
	browser->sendStatusLine();
	browser->sendHeader();
	return true;
}

bool Proxy::transferServerResponseData(){
	cerr << "transferServerResponseData()" << endl;
	cerr << "Content-Length: " << contentLength << endl;
	string* data;
	bool moreData = true;
	if(chunkedTransfer){
		moreData = server->recvChunk();
	}
	else{
		if(contentLength <= 0) return false;
		if(contentLength > 1024){
			server->recvData(1024);
		}
		else{
			server->recvData(contentLength);
		}
	}
	data = server->getData();
	if(!data->length()){
		delete data;
		return false;
	}
	if (!chunkedTransfer) contentLength -= data->length();
	browser->addData(data);
	return moreData;
}
bool Proxy::sendServerResponseData(){
	browser->sendData();
	return true;
}
void Proxy::closeServerConnection(){
	delete server;
	server = nullptr;
}

HeaderField* Proxy::filterHeaderFieldOut(HeaderField* h){
	static ci_string connectionToken;
	ci_string name(h->first.c_str());
	if(name == ci_string("Connection")){
		connectionToken = ci_string(h->second.c_str());
		h->second = "Close";
	}
	else if(name == connectionToken){
		return nullptr;
	}
	else if(name == ci_string("Host")){
		serverHostname = h->second;
	}
	/*else if(name == ci_string("Cookie")){
		return nullptr;
	}*/
	return h;
}

HeaderField* Proxy::filterHeaderFieldIn(HeaderField* h){
	static ci_string connectionToken;
	ci_string name(h->first.c_str());
	if(name == ci_string("Connection")){
		connectionToken = ci_string(h->second.c_str());
		h->second = "Close";
	}
	else if(name == connectionToken){
		return nullptr;
	}
	else if(name == ci_string("Content-Length")){
		stringstream ss;
		ss << h->second;
		ss >> contentLength;
		transferData = true;
		cerr << "Found Content-Length: " << contentLength;
	}
	else if(name == ci_string("Transfer-Encoding")){
		chunkedTransfer = true;
		transferData = true;
		cerr << "Transfer-Encoding: " << h->second << "<<<<<<<<<<<<<<<<<<<<<<<<<<" << endl;
	}
	return h;
}

void Proxy::send501NotImplented(){
	browser->setStatusLine(new string("HTTP/1.1 501 Not Implemented"));
	browser->addHeaderField("Connection", "Close");
	browser->sendStatusLine();
	browser->sendHeader();
}
