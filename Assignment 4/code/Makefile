OBJECTS := $(patsubst %.c,%.o,$(wildcard *.c))
includes := $(wildcard *.h)

all: client-api.a server client

server: server-api.a server.o 
	gcc server.o -L. -lserver-api -o server
	
server-api.a: fs_server.c server_stub.o helper.o mybind.o
	gcc -c fs_server.c
	ar r libserver-api.a fs_server.o server_stub.o helper.o mybind.o

client-api.a: fs_dummy.c client_stub.o helper.o mybind.o
	gcc -c fs_dummy.c
	ar r libclient-api.a fs_dummy.o client_stub.o helper.o mybind.o

client: client-api.a fs_client.o
	gcc fs_client.o -L. -lclient-api -o client

$(OBJECTS): %.o: %.c $(includes)
	gcc -c $< -o $@

clean:
	rm -rf *.out *.o core *.a client server
