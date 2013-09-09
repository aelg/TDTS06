#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstring>
#include <map>
#include <string>

#include "Server.h"
#include "proxy.h"

using namespace std;

void *connectionHandler(void *args){
	Connection *browserConnection;
	string *r;
	string *s = new string("HTTP/1.1 200 OK\r\n\
Content-Length: 36\r\n\
Connection: Close\r\n\
Content-Type: text/html; charset=ASCII\r\n\
\r\n\
<html><body>OHOHOHO!</body></html>\r\n");

	browserConnection = (Connection*) args;

	for(;;){
		r = browserConnection->recvTerminatedString('\n');
		cout << *r;
		if(*r == "\r\n") break;
		else delete r;
	}
	delete r;
	browserConnection->sendString(s);
	shutdown(browserConnection->socket, STOP_RECEIVING);

	delete browserConnection;
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
