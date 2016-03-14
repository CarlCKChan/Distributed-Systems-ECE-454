objects := $(patsubst %.c,%.o,$(wildcard *.c))
includes := $(wildcard *.h)

all: sample_client_app sample_server_app

sample_client_app: libstubs.a sample_client_app.o
	gcc sample_client_app.o -L. -lstubs -o sample_client_app

sample_server_app: libstubs.a sample_server_app.o
	gcc sample_server_app.o -L. -lstubs -o sample_server_app

libstubs.a: server_stub.o client_stub.o helper.o mybind.o
	ar r libstubs.a server_stub.o client_stub.o helper.o mybind.o

$(objects): %.o: %.c $(includes)
	gcc -c $< -o $@

clean:
	rm -rf a.out *.o core *.a *_app
