
CFLAGS=-c -Wall -Wextra -pedantic -O2 -I../
LDFLAGS = 

COMPILER=g++

CCC=$(COMPILER) $(CFLAGS)
LDC=$(COMPILER) $(LDFLAGS)

all: proxy
	
clean:
	rm -rf *.o proxy
	
proxy: Server.o proxy.o Connection.o ../Server.h
	$(LDC) -o proxy Server.o proxy.o Connection.o
Server.o: ../Server.cc ../Server.h ../Connection.h
	$(CCC) ../Server.cc
	
Connection.o: ../Connection.cc ../Connection.h
	$(CCC) ../Connection.cc

proxy.o: ../proxy.cc ../Server.h
	$(CCC) ../proxy.cc
