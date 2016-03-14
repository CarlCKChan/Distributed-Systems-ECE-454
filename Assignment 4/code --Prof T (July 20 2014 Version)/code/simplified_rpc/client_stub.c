/* Mahesh V. Tripunitara
   University of Waterloo
   Part of a simplified RPC implementation
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include "ece454rpc_types.h"

#if 0
#define _DEBUG_1_
#endif

extern void recvbytes(int, void *, ssize_t);
extern void sendbytes(int, void *, ssize_t);

return_type r;

return_type make_remote_call(const char *servernameorip,
	                     const int serverportnumber,
	                     const char *procedure_name,
	                     const int nparams,
			     ...)
{
    /* Setup socket, connect(), etc. */
    struct addrinfo hints, *result;
    hints.ai_family = AF_INET;
    hints.ai_socktype = hints.ai_protocol = hints.ai_flags = 0;

    if(getaddrinfo(servernameorip, NULL, (const struct addrinfo *)(&hints), &result) != 0) {
	perror("getaddrinfo"); exit(1);
    }

    struct sockaddr_in svra;
    memcpy(&svra, result->ai_addr, sizeof(struct sockaddr_in));
    svra.sin_port = htons(serverportnumber);

    freeaddrinfo(result);

    int s = socket(AF_INET, SOCK_STREAM, 0);
    if(s < 0) {
	perror("socket"); exit(1);
    }

    struct sockaddr_in mya;
    memset(&mya, 0, sizeof(struct sockaddr_in));
    mya.sin_family = AF_INET;
    if(bind(s, (const struct sockaddr *)(&mya), sizeof(struct sockaddr_in)) < 0) {
	perror("bind"); exit(1);
    }

    if(connect(s, (const struct sockaddr *)(&svra), sizeof(struct sockaddr_in)) < 0) {
	perror("connect"); exit(1);
    }

    /* Make the remote call */
    int procnamelen = strlen(procedure_name)+1;
    sendbytes(s, (void *)(&procnamelen), sizeof(int));
    sendbytes(s, (void *)procedure_name, procnamelen);
    sendbytes(s, (void *)(&nparams), sizeof(int));

    va_list ap;
    int i;

    va_start(ap, nparams);
    for(i = 0; i < nparams; i++) {
	int psize;
	void *pval;

	psize = va_arg(ap, int);
	pval = va_arg(ap, void *);

#ifdef _DEBUG_1_
	if(psize == sizeof(int)) {
	    printf("make_remote_call(), int-sized arg: %d\n",
		    *(int *)pval);
	    fflush(stdout);
	}
#endif

	sendbytes(s, (void *)(&psize), sizeof(int));
	sendbytes(s, pval, psize);
    }
    va_end(ap);

    /* Get the result */
    recvbytes(s, (void *)(&(r.return_size)), sizeof(int));
    if(r.return_size < 0) {
	/* Error! */
	fprintf(stderr, "Received %d return_size.\n", r.return_size);
	exit(1);
    }
    else if(r.return_size == 0) {
	r.return_val = NULL;
    }
    else {
	r.return_val = (void *)malloc(r.return_size);
	recvbytes(s, r.return_val, r.return_size);
    }

    shutdown(s, SHUT_RDWR); close(s);

    /* Warning! Potential memory leak -- r.return_val */
    return(r);
}
