
CFLAGS=-c -Wall -Wextra -pedantic -O2 -I../
LDFLAGS = -lpthread

COMPILER=g++

CCC=$(COMPILER) $(CFLAGS)
LDC=$(COMPILER) $(LDFLAGS)

all: proxy
	
clean:
	rm -rf *.o proxy
	
proxy: Server.o proxy.o Connection.o HttpConnection.o ../Server.h
	$(LDC) -o proxy Server.o proxy.o Connection.o HttpConnection.o

Server.o: ../Server.cc ../Server.h ../Connection.h
	$(CCC) ../Server.cc
	
Connection.o: ../Connection.cc ../Connection.h
	$(CCC) ../Connection.cc
	
HttpConnection.o: ../HttpConnection.cc ../HttpConnection.h ../Connection.h
	$(CCC) ../HttpConnection.cc

proxy.o: ../proxy.cc ../Server.h
	$(CCC) ../proxy.cc	
	