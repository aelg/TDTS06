
CFLAGS = -c -std=c++11 -Wall -Wextra -pedantic -O2 -I../
LDFLAGS = -lpthread -std=c++11

COMPILER=g++

CCC=$(COMPILER) $(CFLAGS)
LDC=$(COMPILER) $(LDFLAGS)

LIBRARIES=-lpthread

all: proxy
	
clean:
	rm -rf *.o proxy
	
proxy: Server.o main.o Connection.o HttpConnection.o Proxy.o ../Server.h ../Proxy.h ../Connection.h ../HttpConnection.h
	$(LDC) -o proxy Server.o main.o Connection.o HttpConnection.o Proxy.o $(LIBRARIES) 

Server.o: ../Server.cc ../Server.h ../Connection.h
	$(CCC) ../Server.cc
	
Connection.o: ../Connection.cc ../Connection.h
	$(CCC) ../Connection.cc
	
HttpConnection.o: ../HttpConnection.cc ../HttpConnection.h ../Connection.h
	$(CCC) ../HttpConnection.cc
	
Proxy.o: ../Proxy.cc ../Proxy.h ../HttpConnection.h
	$(CCC) ../Proxy.cc

main.o: ../main.cc ../Server.h ../Connection.h ../HttpConnection.h
	$(CCC) ../main.cc	
		