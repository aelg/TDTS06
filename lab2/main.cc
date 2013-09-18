#include <iostream>
#include <fstream>

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
#include "Proxy.h"

using namespace std;

int doExit = false;

struct ThreadData{
	Connection *connection;
	vector<ci_string> *filterWords;
};

void *connectionHandler(void *args){
	ThreadData *threadData;
	Connection *browserConnection;

	threadData = reinterpret_cast<ThreadData*>(args);
	browserConnection = threadData->connection;

	HttpConnection *browserHttpConnection = new HttpConnection(*browserConnection);
	delete browserConnection;

	Proxy p(browserHttpConnection, threadData->filterWords);
	p.run();
	delete threadData;
	return nullptr;
}

void SIGINTHandler(int){
	cerr << "SIGINT received." << endl;
	doExit = true;
}

vector<ci_string> *readFilterWords(){
	char word[200];
	vector<ci_string> *filterWords = new vector<ci_string>; // This will unfortunately not be freed.
	ifstream f("filterWords.txt");
	while(f.getline(word, 200)){
		if(word[0] != '#' && word[0] != 0){ // Skip "# Comments" and empty lines.
			filterWords->push_back(ci_string(word));
		}
	}
	return filterWords;
}

int main(int argc, char *argv[]){
	Server server;
	Connection *newConnection;
	pthread_t threadId;
	map<pthread_t, Connection *> accepted;
	vector<ci_string> *filterWords = readFilterWords();

	struct sigaction sa;
	sa.sa_handler = SIGINTHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, NULL);

	if(argc == 2){
		server.setPort(argv[1]);
	}
	server.init();

	for(;!doExit;){
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



		pthread_create(&threadId, NULL, connectionHandler, (void*) new ThreadData{newConnection, filterWords});
		//pthread_join(threadId, NULL);
		//accepted[threadId] = newConnection;

	}
	server.stopListening();
	cout << "Waiting for all connections to finish." << endl;
	//for(auto it = accepted.begin(); it != accepted.end(); ++it){
	//	pthread_join(it->first, NULL);
	//}
	cout << "Exiting." << endl;
	pthread_exit(nullptr);
}
