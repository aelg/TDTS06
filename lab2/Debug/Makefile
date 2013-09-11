
CFLAGS = -c -g -std=c++11 -Wall -Wextra -pedantic -I../
LDFLAGS = -g -std=c++11

COMPILER=g++

CCC=$(COMPILER) $(CFLAGS)
LDC=$(COMPILER) $(LDFLAGS)

LIBRARIES=-lpthread

all: proxy
	
clean:
	rm -rf *.o proxy
	
proxy: Server.o proxy.o Connection.o HttpConnection.o ../Server.h
	$(LDC) -o proxy Server.o proxy.o Connection.o HttpConnection.o $(LIBRARIES) 

Server.o: ../Server.cc ../Server.h ../Connection.h
	$(CCC) ../Server.cc
	
Connection.o: ../Connection.cc ../Connection.h
	$(CCC) ../Connection.cc
	
HttpConnection.o: ../HttpConnection.cc ../HttpConnection.h ../Connection.h
	$(CCC) ../HttpConnection.cc

proxy.o: ../proxy.cc ../Server.h ../Connection.h ../HttpConnection.h
	$(CCC) ../proxy.cc	
	