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
																				contentLength(0), transferResponseData(false),
																				transferRequestData(false), chunkedTransfer(false){}

Proxy::~Proxy() {
	if(browser) delete browser;
	if(server) delete server;
}

void Proxy::run(){
	try{
		for(int i = 0; i<1; ++i){
			if(!readBrowserRequest()) break;
			//cerr << "1" << endl;
			if(!transferBrowserRequest()) break;
			//cerr << "2" << endl;
			if(!setupServerConnection()) break;
			//cerr << "3" << endl;
			if(!sendBrowserRequest()) break;
			//cerr << "4" << endl;
			if(transferRequestData){
				while(transferData(browser, server)){
					sendBrowserRequestData();
				}
			}
			//cerr << "5" << endl;
			if(!readServerResponse()) break;
			//cerr << "6" << endl;
			if(!transferServerResponseHeader()) break;
			//cerr << "7" << endl;
			if(!sendServerResponseHeader()) break;
			//cerr << "8" << endl;
			if(transferResponseData){
				while(transferData(server, browser)){
					sendServerResponseData();
				}
			}
			//cerr << "9" << endl;
		}
	}catch(ConnectionException &e){
		cerr << "Caught ConnectionException in Proxy::run: " << e.what() << endl;
	}catch(HttpConnectionException &e){
		cerr << "Caught HttpConnectionException in Proxy::run: " << e.what() << endl;
	}catch(ProxyException &e){
		cerr << "Caught ProxyException in Proxy::run: " << e.what() << endl;
	}
	closeServerConnection();
}

bool Proxy::readBrowserRequest(){
	HttpConnection::ReceivedType r;
	try{
		r = browser->recvStatusLine();
	}catch(HttpConnectionException &e){
		cerr << "Connection closed." << endl;
		return false;
		//throw ProxyException(string("readBrowserRequest catched: ") + e.what(),
		//										 ProxyException::UNKNOWN_ERROR);
	}
	browser->recvHeader();
	switch(r){
	case HttpConnection::GET_REQUEST:
	case HttpConnection::POST_REQUEST:
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
	contentLength = 0;
	chunkedTransfer = false;

	server = new HttpConnection(Connection());

	statusLine = browser->getStatusLine();
	//cerr << "Status Line: " << *statusLine << endl;

	if((httpPos = statusLine->find("http://")) != string::npos){
		statusLine->erase(httpPos, statusLine->find_first_of('/', httpPos+7) - httpPos);
	}
	server->setStatusLine(statusLine);

	while((h = browser->getHeaderField())){
		h = filterHeaderFieldOut(h);
		if(h)server->addHeaderField(h);
	}
	server->addHeaderField("Connection", "Close");
	return true;
}

bool Proxy::setupServerConnection(){
	if(serverHostname.empty()){
		throw ProxyException("setupServerConnection: Host header not found.",
				ProxyException::HOST_HEADER_NOT_FOUND);
	}
	int sockfd, rv;
	struct addrinfo hints, *servinfo, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	//cerr << "Connecting to: '" << serverHostname.c_str() << '\''<< endl;
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
			shutdown(sockfd, SHUT_RDWR);
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
	Connection newConnection(sockfd);
	server->setConnection(newConnection);
	return true;
}
bool Proxy::sendBrowserRequest(){
	server->sendStatusLine();
	server->sendHeader();
	return true;
}
bool Proxy::sendBrowserRequestData(){
	try{
			server->sendData();
		}catch(ConnectionException &e){
			if (e.error_number == ConnectionException::BROKEN_PIPE) cerr << "Aborted by server." << endl;
			else cerr << "Send error" << e.what() << endl;
			throw;
		}
	return true;
}
bool Proxy::readServerResponse(){
	HttpConnection::ReceivedType r;
	r = server->recvStatusLine();
	server->recvHeader();
	contentLength = 0;
	transferResponseData = false;
	chunkedTransfer = false;
	switch(r){
		case HttpConnection::GET_REQUEST:
		case HttpConnection::POST_REQUEST:
			throw ProxyException("Received GET request from server.", ProxyException::BAD_SERVER);
			break;
		case HttpConnection::RESPONSE:
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
	//cerr << "transferServerResponseHeader" << endl;
	string *statusLine;
	HeaderField *h;

	statusLine = server->getStatusLine();
	browser->setStatusLine(statusLine);

	while((h = server->getHeaderField())){
		h = filterHeaderFieldIn(h);
		if(h)browser->addHeaderField(h);
	}
	browser->addHeaderField("Connection", "Close");
	return true;
}

bool Proxy::sendServerResponseHeader(){
	//cerr << "sendServerResponseHeader()" << endl;
	//cerr << "transferResponseData: " << transferResponseData << endl;
	browser->sendStatusLine();
	browser->sendHeader();
	if(server->getStatusCode() != 200 && transferResponseData) cerr << "OMG!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!111111" << endl;
	return true;
}

bool Proxy::transferData(HttpConnection* from, HttpConnection *to){
	//cerr << "transferServerResponseData()" << endl;
	//cerr << "Content-Length: " << contentLength << endl;
	string* data;
	if(!chunkedTransfer && contentLength <= 0){
		return false;
	}
	if(chunkedTransfer){
		if(!from->recvChunk(1024)){
			chunkedTransfer = false;
			contentLength = 0;
		}
	}
	else{
		//cerr << "ContentLength is now: " << contentLength << endl;
		if(contentLength > 1024){
			from->recvData(1024);
		}
		else{
			from->recvData(contentLength);
		}
	}
	data = from->getData();
	//cerr << "data length: " << data->length() << endl;
	/*if(!data->length()){
		delete data;
		return false;
	}*/
	if (!chunkedTransfer){
		contentLength -= data->length();
	}

		//cerr << "Adding chunk of size: " << data->length() << endl;

	to->addData(data);
	return true;
}

bool Proxy::sendServerResponseData(){
	//cerr << "sendData" << endl;
	try{
		browser->sendData();
	}catch(ConnectionException &e){
		if (e.error_number == ConnectionException::BROKEN_PIPE) cerr << "Aborted by browser." << endl;
		else cerr << "Send error" << e.what() << endl;
		throw;
	}
	return true;
}
void Proxy::closeServerConnection(){
	if(browser){
		browser->closeConnection();
		delete browser;
		browser = nullptr;
	}
	if(server){
		server->closeConnection();
		delete server;
		server = nullptr;
	}

}

HeaderField* Proxy::filterHeaderFieldOut(HeaderField* h){
	static ci_string connectionToken;
	ci_string name(h->first.c_str());
	if(name == ci_string("Connection")){
		connectionToken = ci_string(h->second.c_str());
		h->second = "Close";
		delete h;
		return nullptr;
	}
	else if(name == connectionToken){
		delete h;
		return nullptr;
	}
	else if(name == ci_string("Host")){
		serverHostname = h->second;
	}
	else if(name == ci_string("Content-Length")){
		stringstream ss;
		ss << h->second;
		ss >> contentLength;
		if(contentLength > 0) transferRequestData = true;
		//cerr << "Found Content-Length: " << contentLength;
	}
	else if(name == ci_string("Transfer-Encoding")){
		chunkedTransfer = true;
		transferRequestData = true;
		//cerr << "Transfer-Encoding: " << h->second << "<<<<<<<<<<<<<<<<<<<<<<<<<<" << endl;
	}
	else if(name == ci_string("Transfer-coding")){
		//cerr << "Transfer-coding <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << endl;
	}
	return h;
}

HeaderField* Proxy::filterHeaderFieldIn(HeaderField* h){
	static ci_string connectionToken;
	ci_string name(h->first.c_str());
	if(name == ci_string("Connection")){
		connectionToken = ci_string(h->second.c_str());
		h->second = "Close";
		delete h;
		return nullptr;
	}
	else if(name == connectionToken){
		delete h;
		return nullptr;
	}
	else if(name == ci_string("Content-Length")){
		stringstream ss;
		ss << h->second;
		ss >> contentLength;
		if(contentLength > 0) transferResponseData = true;
		//cerr << "Found Content-Length: " << contentLength << endl;
	}
	else if(name == ci_string("Transfer-Encoding")){
		chunkedTransfer = true;
		transferResponseData = true;
		//cerr << "Transfer-Encoding: " << h->second << "<<<<<<<<<<<<<<<<<<<<<<<<<<" << endl;
	}
	return h;
}

void Proxy::send501NotImplented(){
	browser->setStatusLine(new string("HTTP/1.1 501 Not Implemented"));
	browser->addHeaderField("Connection", "Close");
	browser->sendStatusLine();
	browser->sendHeader();
}
