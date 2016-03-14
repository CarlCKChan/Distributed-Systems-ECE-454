OBJECTS := $(patsubst %.c,%.o,$(wildcard *.c))

fs_client: fs_client.o fs_dummy.o

$(OBJECTS): %.o: %.c ece454_fs.h
	gcc -c $< -o $@

clean:
	rm -rf a.out *.o core *.a fs_client
