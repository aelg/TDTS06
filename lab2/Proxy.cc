/*
 * Proxy.cc
 *
 *  Created by: Linus Mellberg
 *  12 sep 2013
 */

#include <string>

#include "Proxy.h"

using namespace std;

Proxy::Proxy(HttpConnection *browser) : browser(browser), server(nullptr) {}

Proxy::~Proxy() {
	if(browser) delete browser;
	if(server) delete server;
}

void Proxy::readBrowserRequest(){
	browser->recvStatusLine();
	browser->recvHeader();
	browser->recvData();
}

void Proxy::transferBrowserRequest(){
	string* statusLine;
	HeaderField *h;

	server = new HttpConnection(Connection());

	statusLine = browser->getStatusLine();
	server->setStatusLine(statusLine);

	while((h = browser->getHeaderField())){
		filterHeaderFieldOut(h);
		server->addHeaderField(h);
	}
};

void Proxy::setupServerConnection(){
	if(serverHostname.empty()){
		throw ProxyException("setupServerConnection: Host header not found.",
				ProxyException::HOST_HEADER_NOT_FOUND);
	}

};
void Proxy::sendBrowserRequest(){};
void Proxy::readServerResponse(){};
bool Proxy::transferServerResponse(){return false;};
void Proxy::sendServerResponse(){};
void Proxy::closeServerConnection(){};

void Proxy::filterHeaderFieldOut(HeaderField* h){};

