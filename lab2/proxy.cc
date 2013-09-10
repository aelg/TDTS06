#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstring>
#include <map>
#include <string>

#include "Server.h"
#include "Connection.h"
#include "HttpConnection.h"

using namespace std;

void *connectionHandler(void *args){
	Connection *browserConnection;
	string *r;

	browserConnection = (Connection*) args;

	HttpConnection *browserHttpConnection = new HttpConnection(*browserConnection);

	browserHttpConnection->setStatusLine(new string("HTTP/1.1 200 OK"));
	browserHttpConnection->addHeaderField("Connection", "Close");
	browserHttpConnection->addHeaderField("Content-Type", "text/html; charset=ASCII");
	browserHttpConnection->addData(new string("<html><body>OHOHOHO!</body></html>\r\n"));
	browserHttpConnection->addContentLength();

	for(;;){
		r = browserConnection->recvTerminatedString('\n');
		cout << *r;
		if(*r == "\r\n") break;
		else delete r;
	}
	delete r;
	browserHttpConnection->sendStatusLine();
	browserHttpConnection->sendHeader();
	browserHttpConnection->sendData();

	delete browserConnection;
	delete browserHttpConnection;
	return NULL;
}

int main(){
	Server server("8080");
	Connection *newConnection;
	pthread_t threadId;
	map<pthread_t, Connection *> accepted;

	for(int i = 0; i < 10; ++i){
		newConnection = server.acceptNew();

		// Fork.
		pthread_create(&threadId, NULL, connectionHandler, (void*) newConnection);
		accepted[threadId] = newConnection;
	}
	cout << "Exiting." << endl;
}
