/* Mahesh V. Tripunitara
   University of Waterloo
   Part of a simplified RPC implementation
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

#include "ece454rpc_types.h"

void testsort(char *argv[]) {
    int fd = open("/dev/urandom", O_RDONLY);
    const unsigned int maxarsize = 10;
    unsigned int arsize;
    read(fd, &arsize, sizeof(unsigned int));
    arsize = arsize%maxarsize + 1;

    int *ar = (int *)calloc(arsize, sizeof(int));
    read(fd, ar, arsize*sizeof(int));
    close(fd);

    int i;
    printf("Integer array before sort:\n\t");
    for(i = 0; i < arsize; i++) {
	if(i > 0 && (i%4 == 0)) {
	    printf("\n\t");
	}

	ar[i] %= (INT_MAX/2);
	printf("%d", ar[i]);
	if(i < arsize-1) printf(", ");
    }
    printf("\n");

    return_type ans;
    ans = make_remote_call(argv[1], atoi(argv[2]),
	    "max_of_integer_array", 1,
	    arsize*sizeof(int), ar);

    free(ar);
    ar = (int *)(ans.return_val);

    printf("Integer array after sort:\n\t");
    for(i = 0; i < arsize; i++) {
	if(i > 0 && (i%4 == 0)) {
	    printf("\n\t");
	}
	printf("%d", ar[i]);
	if(i < arsize-1) printf(", ");
    }
    printf("\n");
    free(ar);
}

void testconcat(char *argv[]) {
    char *s[5];
    const int maxslen = 10;
    int fd = open("/dev/urandom", O_RDONLY);

    printf("testconcat(), input strings:\n");
    int i;
    for(i = 0; i < 5; i++) {
	unsigned int slen;
	read(fd, &slen, sizeof(unsigned int));
	slen %= maxslen; slen++;
	s[i] = (char *)calloc(slen+1, sizeof(char));
	s[i][slen] = (char)0;
	read(fd, s[i], slen);
	int j;
	for(j = 0; j < slen; j++) {
	    s[i][j] = ((unsigned int)(s[i][j]))%30 + 'a';
	}

	printf("\t%d, %s\n", slen, s[i]);
    }

    return_type ans = make_remote_call(argv[1],
	    atoi(argv[2]),
	    "concatenate_five_strings", 5,
	    strlen(s[0])+1, s[0],
	    strlen(s[1])+1, s[1],
	    strlen(s[2])+1, s[2],
	    strlen(s[3])+1, s[3],
	    strlen(s[4])+1, s[4]);

    for(i = 0; i < 5; i++) {
	free(s[i]);
    }

    printf("Result: %d, %s\n", ans.return_size,
	    (char *)(ans.return_val));

    free(ans.return_val);
}

int main(int argc, char *argv[])
{
    if(argc < 3) {
	printf("usage: %s <server ip/name> <server port>\n", argv[0]);
	return 0;
    }
    return_type ans;

    ans = make_remote_call(argv[1], atoi(argv[2]), "nothing", 0);

    printf("nothing():: ans.return_val = %d\n", ans.return_size);

    int a = -10, b = 20;
    ans = make_remote_call(argv[1],
		       atoi(argv[2]),
		       "addtwo", 2,
	               sizeof(int), (void *)(&a),
	               sizeof(int), (void *)(&b));
    int i = *(int *)(ans.return_val);
    printf("addtwo(%d, %d) = %d\n", a, b, i);

    ans = make_remote_call(argv[1], atoi(argv[2]),
	    "pickFirst", 2,
	    sizeof(int), (void *)(&a),
	    sizeof(int), (void *)(&b));

    if(ans.return_size > 0) {
	printf("pickFirst(%d, %d) = %d\n", a, b,
		*(int *)(ans.return_val));
    }
    else {
	printf("return_size is 0!\n");
    }

    testsort(argv);

    /* Does addtwo still work?*/
    a = 200; b = -12;
    ans = make_remote_call(argv[1],
		       atoi(argv[2]),
		       "addtwo", 2,
	               sizeof(int), (void *)(&a),
	               sizeof(int), (void *)(&b));
    i = *(int *)(ans.return_val);
    printf("addtwo(%d, %d) = %d\n", a, b, i);

    testconcat(argv);

    return 0;
}
