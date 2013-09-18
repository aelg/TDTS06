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

/* Thread handler */
void *connectionHandler(void *args){
	ThreadData *threadData;
	Connection *browserConnection;

	threadData = reinterpret_cast<ThreadData*>(args);
	browserConnection = threadData->connection;

	HttpConnection *browserHttpConnection = new HttpConnection(*browserConnection);
	delete browserConnection;

	/* Start the proxy for the connection */
	Proxy p(browserHttpConnection, threadData->filterWords);
	p.run();
	delete threadData;
	return nullptr;
}

/* Handler for SIGINT signal, which is caused by ctrl+c.
 * Setting doExit will cause main() to stop accepting connections and exit.
 * The proxy will exit when all threads are done with their connections.
 */
void SIGINTHandler(int){
	cerr << "SIGINT received." << endl;
	doExit = true;
}

vector<ci_string> *readFilterWords(const char * fileName){
	char word[200];
	/* This will unfortunately not be freed. This is because the main thread
	 * don't wait for the threads to exit and thus can't free this.
	 * When a thread ends it has no way of knowing if it's the last.
	 * This is no problem though since there will only be one of these
	 * and it won't clog the memory.
	 */
	vector<ci_string> *filterWords = new vector<ci_string>;
	ifstream f(fileName);
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

	/* Read filtered words from textfile. */
	vector<ci_string> *filterWords = readFilterWords("filterWords.txt");

	/* Setup handling of INTSIG (ctrl+c) */
	struct sigaction sa;
	sa.sa_handler = SIGINTHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
	/* Setup handling for SIGPIPE if this isn't done all threads
	 * will die when SIGPIPE is received.
	 */
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

		// These threads won't be joined, so detach them to free resources when they exit.
		pthread_detach(threadId);

	}
	server.stopListening();
	cout << "Exiting." << endl;
	cout << "The proxy will stop after all current connections has been closed." << endl;
	pthread_exit(nullptr);
}
