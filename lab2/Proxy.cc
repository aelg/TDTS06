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

Proxy::Proxy(HttpConnection *browser) : browser(browser), server(nullptr), contentLength(0) {}

Proxy::~Proxy() {
	if(browser) delete browser;
	if(server) delete server;
}

void Proxy::run(){
	readBrowserRequest();
	transferBrowserRequest();
	setupServerConnection();
	sendBrowserRequest();
	readServerResponse();
	transferServerResponseHeader();
	sendServerResponseHeader();
	while(transferServerResponseData()){
		sendServerResponseData();
	}
	closeServerConnection();
}

void Proxy::readBrowserRequest(){
	/*browser->recvStatusLine();
	browser->recvHeader();
	browser->recvData();*/
}

void Proxy::transferBrowserRequest(){
	string* statusLine;
	HeaderField *h;
	serverHostname = "";

	server = new HttpConnection(Connection());

	statusLine = browser->getStatusLine();
	server->setStatusLine(statusLine);

	while((h = browser->getHeaderField())){
		filterHeaderFieldOut(h);
		server->addHeaderField(h);
	}
}

void Proxy::setupServerConnection(){
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
	}
	if(p == nullptr){
		throw ProxyException("Remote server setup failed, failed to connect.",
						ProxyException::SETUP_SERVER_CONNECTION_ERROR);
	}
	freeaddrinfo(servinfo);
	addr = *reinterpret_cast<sockaddr*>(p);
	Connection newConnection(sockfd, addr);
	server->setConnection(newConnection);
}
void Proxy::sendBrowserRequest(){
	server->sendStatusLine();
	server->sendHeader();
}
void Proxy::readServerResponse(){
	/*server->recvStatusLine();
	server->recvHeader();*/
}
void Proxy::transferServerResponseHeader(){
	string *statusLine;
	HeaderField *h;
	contentLength = 0;

	statusLine = server->getStatusLine();
	browser->setStatusLine(statusLine);

	while((h = server->getHeaderField())){
		filterHeaderFieldIn(h);
	}
}

void Proxy::sendServerResponseHeader(){
	server->sendStatusLine();
	server->sendHeader();
}

bool Proxy::transferServerResponseData(){
	if(contentLength == 0) return false;
	string* data;
	server->getData();
	browser->addData(data);
	contentLength -= data->length();
	return true;
}
void Proxy::sendServerResponseData(){
	server->sendData();
}
void Proxy::closeServerConnection(){
	delete server;
}

void Proxy::filterHeaderFieldOut(HeaderField* h){
	static ci_string connectionToken;
	ci_string name(h->first.c_str());
	if(name == ci_string("Connection")){
		connectionToken = ci_string(h->second.c_str());
		h->second = "Close";
	}
	else if(name == connectionToken){
		return;
	}
	else if(name == ci_string("Host")){
		serverHostname = h->first;
	}
}

void Proxy::filterHeaderFieldIn(HeaderField* h){
	static ci_string connectionToken;
	ci_string name(h->first.c_str());
	if(name == ci_string("Connection")){
		connectionToken = ci_string(h->second.c_str());
		h->second = "Close";
	}
	else if(name == connectionToken){
		return;
	}
	else if(name == ci_string("Content-Length")){
		stringstream ss;
		ss << h->second;
		ss >> contentLength;
	}
}
