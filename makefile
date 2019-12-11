FLAG=-pthread

all:
	g++ -c Socket.cpp -o Socket.o
	g++ Socket.o OX_server.cpp $(FLAG) -o OX_server
	g++ Socket.o OX_client.cpp $(FLAG) -o OX_client
clean:
	rm -f server client

