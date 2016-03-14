objects := $(patsubst %.c,%.o,$(wildcard *.c))

client.out: libstubs.a myclient.o
	gcc myclient.o -g -L. -lstubs -o client.out
	
server.out: libstubs.a myserver.o
	gcc myserver.o -g -L. -lstubs -o server.out

libstubs.a: server_stub.o client_stub.o
	ar r libstubs.a server_stub.o client_stub.o

$(objects): %.o: %.c ece454rpc_types.h
	gcc -c $< -o $@

clean:
	rm -rf a.out *.o core *.a
