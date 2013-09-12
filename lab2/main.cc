#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <cstring>
#include <map>
#include <string>

#include "Server.h"
#include "Connection.h"
#include "HttpConnection.h"

using namespace std;

int doExit = false;

void *connectionHandler(void *args){
	Connection *browserConnection;
	string *r = 0;

	browserConnection = (Connection*) args;

	HttpConnection *browserHttpConnection = new HttpConnection(*browserConnection);

	for(;browserConnection->isGood() && !doExit;){
		browserHttpConnection->setStatusLine(new string("HTTP/1.1 200 OK"));
		browserHttpConnection->addHeaderField("Connection", "Keep-Alive");
		browserHttpConnection->addHeaderField("Keep-Alive", "timeout=10");
		browserHttpConnection->addHeaderField("Content-Type", "text/html; charset=ASCII");
		browserHttpConnection->addData(new string("<html><body>OHOHOHO!</body></html>\r\n"));
		browserHttpConnection->addContentLength();

		for(;browserConnection->isGood();){
			r = browserConnection->recvTerminatedString('\n');
			cout << *r;
			if(*r == "\r\n") break;
			else{
				delete r;
				r = nullptr;
			}
		}
		if (r) delete r;
		browserHttpConnection->sendStatusLine();
		browserHttpConnection->sendHeader();
		browserHttpConnection->sendData();
	}

	delete browserConnection;
	delete browserHttpConnection;
	return NULL;
}

void SIGINTHandler(int){
	cerr << "SIGINT received." << endl;
	doExit = true;
}

int main(){
	Server server("8080");
	Connection *newConnection;
	pthread_t threadId;
	map<pthread_t, Connection *> accepted;

	struct sigaction sa;
	sa.sa_handler = SIGINTHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, NULL);

	for(;;){
		try{
			newConnection = server.acceptNew();
		}catch(ServerException &e){
			if(e.errorNumber == e.ACCEPT_INTERRUPTED){
				cerr << "Accept interrupted.\nStopped receiving new connections." << endl;
				break;
			}
			else{
				cerr << "ServerException caught: " << e.what() << endl;
				break;
			}
		}
		// Fork.

		pthread_create(&threadId, NULL, connectionHandler, (void*) newConnection);
		accepted[threadId] = newConnection;

	}
	server.stopListening();
	cout << "Waiting for all connections to finish." << endl;
	for(auto it = accepted.begin(); it != accepted.end(); ++it){
		pthread_join(it->first, NULL);
	}
	cout << "Exiting." << endl;
}
