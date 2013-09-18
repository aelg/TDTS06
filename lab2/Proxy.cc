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
#include <vector>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "Proxy.h"

int const STOP_RECEIVING = 0;

using namespace std;

Proxy::Proxy(HttpConnection *browser, vector<ci_string> *filterWords) :
		browser(browser), server(nullptr),
		contentLength(0), transferResponseData(false),
		transferRequestData(false), chunkedTransfer(false),
		shouldBeFiltered(false), savedAcceptEncoding(nullptr),
		filterBuffer(nullptr), filterWords(filterWords){}

Proxy::~Proxy() {
	if(browser) delete browser;
	if(server) delete server;
	if(filterBuffer) delete filterBuffer;
}

void Proxy::run(){
	try{
		for(int i = 0; i<1; ++i){
			if(!readBrowserRequest()) break;
			if(!transferBrowserRequest()) break;
			if(!setupServerConnection()) break;
			if(!sendBrowserRequest()) break;
			if(transferRequestData){
				while(transferData(browser, server)){
					sendBrowserRequestData();
				}
			}
			if(!readServerResponse()) break;
			if(!transferServerResponseHeader()) break;
			if(shouldBeFiltered){
				if(transferResponseData){
					// Read the data but don't send it.
					while(transferData(server,browser));

					if(filterBuffer){
						if(filterData()){ // Filter the data.
							if(!sendServerResponseHeader()) break;
							browser->addData(filterBuffer);
							if(!sendServerResponseData()) break;
						}
						else{
							send303SeeOther("http://www.ida.liu.se/~TDTS04/labs/2011/ass2/error2.html");
							break;
						}
					} // if(filterBuffer)
					else{ // No filterBuffer.
						sendServerResponseHeader();
					}
				}// if(transferResponseData)
				else{// No data just send the header.
					if(!sendServerResponseHeader()) break;
				}
			} // if(shouldBeFiltered)
			else{ // No filtering send response header and data.
				if(!sendServerResponseHeader()) break;
				while(transferData(server, browser)){
					sendServerResponseData();
				}
			}
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
		cerr << "Connection closed.\nFil: " << __FILE__ << "\nRad: " << __LINE__ << endl;
		throw;
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

	// Instantiate server object.
	server = new HttpConnection(Connection());

	statusLine = browser->getStatusLine();
	ci_string ci_statusLine(statusLine->c_str());
	for(auto it = filterWords->begin(); it != filterWords->end(); ++it){
		if(ci_statusLine.find(*it) != string::npos){
			send303SeeOther("http://www.ida.liu.se/~TDTS04/labs/2011/ass2/error1.html");
			delete statusLine;
			return false;
		}
	}

	// Remove server address from request.
	if((httpPos = statusLine->find("http://")) != string::npos){
		statusLine->erase(httpPos, statusLine->find_first_of('/', httpPos+7) - httpPos);
	}
	server->setStatusLine(statusLine);

	// Read and filter headers.
	while((h = browser->getHeaderField())){
		h = filterHeaderFieldOut(h);
		if(h)server->addHeaderField(h);
	}

	// Add Connection: Close header.
	server->addHeaderField("Connection", "Close");

	/* Set Accept-Encoding header field to empty if the Content-Type header field
	 * indicates that the header should be filtered. This will force the server to not
	 * compress the data. Bad for bandwidth usage, but we can filter more data
	 * without implementing decompressors.
	 */
	if(savedAcceptEncoding){
		if(shouldBeFiltered){
			savedAcceptEncoding->second = "";
		}
		server->addHeaderField(savedAcceptEncoding);
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

	// Standard socket stuff.
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
	contentLength = -1;
	transferResponseData = false;
	chunkedTransfer = false;
	switch(r){
		case HttpConnection::GET_REQUEST:
		case HttpConnection::POST_REQUEST:
			throw ProxyException("Received GET request from server.", ProxyException::BAD_SERVER);
			break;
		case HttpConnection::RESPONSE:
			if(server->getStatusCode() == 200){
				transferResponseData = true;
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
	string *statusLine;
	HeaderField *h;

	statusLine = server->getStatusLine();
	browser->setStatusLine(statusLine);

	// Filter incoming headers.
	while((h = server->getHeaderField())){
		h = filterHeaderFieldIn(h);
		if(h)browser->addHeaderField(h);
	}
	// Add Connection: Close
	browser->addHeaderField("Connection", "Close");
	return true;
}

bool Proxy::sendServerResponseHeader(){
	browser->sendStatusLine();
	browser->sendHeader();
	if(server->getStatusCode() != 200 && transferResponseData) cerr << "OMG!!!!!!111111 " << server->getStatusCode() << endl;
	return true;
}

bool Proxy::transferData(HttpConnection* from, HttpConnection *to){
	string* data;
	if(!chunkedTransfer && contentLength == 0){
		// We are done.
		return false;
	}
	if(chunkedTransfer){
		if(!from->recvChunk(1024)){
			chunkedTransfer = false;
			contentLength = 0;
		}
	}
	else{
		if(contentLength > 1024 || contentLength == -1){
			from->recvData(1024);
		}
		else{
			from->recvData(contentLength);
		}
	}
	data = from->getData();

	// If we will filter add to filterBuffer.
	if(data && shouldBeFiltered){
		if(!filterBuffer) filterBuffer = new string();
		filterBuffer->append(*data);
	}
	if (!chunkedTransfer && contentLength != -1){
		contentLength -= data->length();
	}

	if(data){
		to->addData(data);
		return true;
	}
	else return false;
}

bool Proxy::sendServerResponseData(){
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
	}
	else if(name == ci_string("Transfer-Encoding")){
		// Checking for chunked transfer.
		if(ci_string(h->second.c_str()) != ci_string("identity")){
			chunkedTransfer = true;
			transferRequestData = true;
		}
	}
	else if(name == ci_string("Accept-Encoding")){
		/* Remove but save, we don't know yet if we should filter or not.
		 * If we're filtering the value of this header is changed to be empty.
		 */
		savedAcceptEncoding = h;
		h = nullptr;
	}
	else if(name == ci_string("Accept")){
		if(h->second.find("text") != string::npos){
			shouldBeFiltered = true;
		}
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
	}
	else if(name == ci_string("Content-Type")){
		cerr << h->first << ": " << h->second << endl;
		if(h->second.find("text") != string::npos){
			shouldBeFiltered = true;
			cerr << "Will filter." << endl;
		}
		else{
			shouldBeFiltered = false;
		}
	}
	return h;
}

bool Proxy::filterData(){
	ci_string ci_filterBuffer(filterBuffer->c_str());
	for(auto it = filterWords->begin(); it != filterWords->end(); ++it){
		if(ci_filterBuffer.find(*it) != string::npos) return false;
	}
	return true;
}

void Proxy::send501NotImplented(){
	browser->setStatusLine(new string("HTTP/1.1 501 Not Implemented"));
	browser->addHeaderField("Connection", "Close");
	browser->sendStatusLine();
	browser->sendHeader();
}

void Proxy::send303SeeOther(string location){
	browser->setStatusLine(new string("HTTP/1.1 303 SeeOther"));
	browser->addHeaderField("Location", location);
	browser->addHeaderField("Connection", "Close");
	browser->sendStatusLine();
	browser->sendHeader();
}
