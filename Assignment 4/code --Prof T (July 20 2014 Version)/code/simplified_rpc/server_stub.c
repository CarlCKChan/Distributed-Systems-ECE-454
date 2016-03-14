/* Mahesh V. Tripunitara
   University of Waterloo
   Part of a simplified RPC implementation
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <signal.h>
#include "ece454rpc_types.h"

#if 0
#define _DEBUG_1_
#endif

extern uint32_t getPublicIPAddr();
extern void recvbytes(int, void *, ssize_t);
extern void sendbytes(int, void *, ssize_t);

/* Linked list of registered functions */
struct fn {
    char *fname;
    fp_type fp;
    int nparams;
    struct fn *next;
};

struct fn *fnp = NULL; /* Database of registered functions.
                          Warning: almost no error checking!
			  E.g., a function may be registered
			  twice and we wouldn't know it. */

void printRegisteredProcedures() {
    printf("Registered procedures:\n"); fflush(stdout);
    struct fn *tmp;
    for(tmp = fnp; tmp != NULL; tmp = tmp->next) {
	printf("\t0x%08x, %s, %d\n", (unsigned int)tmp,
		tmp->fname, tmp->nparams);
	fflush(stdout);
    }

}

bool register_procedure(const char *procedure_name,
		        const int nparams,
		        fp_type fnpointer)
{
    if(procedure_name == NULL) return false;

    struct fn *newfn = (struct fn *)malloc(sizeof(struct fn));
    newfn->next = NULL;
    newfn->fname = (char *)calloc(strlen(procedure_name)+1, sizeof(char));
    strcpy(newfn->fname, procedure_name);
    newfn->fp = fnpointer;
    newfn->nparams = nparams;
    newfn->next = fnp;
    fnp = newfn;

    return true;
}

void recvCall(int s, char **pfname, int *pnparams, arg_type **pa) {
    int fnamelen;

#ifdef _DEBUG_1_
    printf("In recvCall()\n"); fflush(stdout);
#endif

    recvbytes(s, (void *)(&fnamelen), sizeof(int));

#ifdef _DEBUG_1_
    printf("recvCall(), fnamelen = %d\n", fnamelen); fflush(stdout);
#endif

    if(fnamelen <= 0) {
	fprintf(stderr, "fnamelen = %d\n!", fnamelen);
	exit(1);
    }

    *pfname = (char *)calloc(fnamelen + 1, sizeof(char));
    memset(*pfname, 0, fnamelen+1);
    recvbytes(s, (void *)(*pfname), fnamelen);

#ifdef _DEBUG_1_
    printf("recvCall(), fname = %s\n", *pfname); fflush(stdout);
#endif

    recvbytes(s, (void *)(pnparams), sizeof(int));

#ifdef _DEBUG_1_
    printf("recvCall(), *pnparams = %d\n", *pnparams); fflush(stdout);
#endif

    if((*pnparams) < 0) {
	fprintf(stderr, "*pnparams = %d\n!", *pnparams);
	exit(1);
    }

    int i;
    for(i = 0; i < *pnparams; i++) {
	arg_type *newarg = (arg_type *)malloc(sizeof(arg_type));
	recvbytes(s, (void *)(&(newarg->arg_size)), sizeof(int));
	if((newarg->arg_size) <= 0) {
	    fprintf(stderr, "newarg[%d]->arg_size = %d\n!",
		    i, newarg->arg_size);
	    exit(1);
	}

	newarg->arg_val = (void *)malloc(newarg->arg_size);
	recvbytes(s, (void *)(newarg->arg_val), newarg->arg_size);

	newarg->next = NULL;
	if(i == 0) {
	    *pa = newarg;
	}
	else {
	    /* insert at the end */
	    arg_type *tmp;
	    for(tmp = *pa; tmp->next != NULL; tmp = tmp->next) ;
	    tmp->next = newarg;
	}
    }

    if(*pnparams <= 0) {
	*pa = NULL;
    }
}

void makeCall(char *fname, int nparams, arg_type *a, return_type *r) {
    if(r == NULL) {
	fprintf(stderr, "makeCall() -- null r!\n");
	exit(1);
    }

    if(fname == NULL) {
	fprintf(stderr, "makeCall() with null fname!\n");
	exit(1);
    }

#ifdef _DEBUG_1_
	printf("makeCall(), about to look for %s\n", fname); fflush(stdout);
#endif

    struct fn *tmp;
    for(tmp = fnp; tmp != NULL; tmp = tmp->next) {
#ifdef _DEBUG_1_
	printf("makeCall(), tmp: 0x%08x\n", (unsigned int)tmp);
	fflush(stdout);
#endif
	if(!strcmp(fname, tmp->fname) && nparams == tmp->nparams) {
	    break;
	}
    }

#ifdef _DEBUG_1_
	printf("makeCall(), stopped looking for %s\n", fname); fflush(stdout);
#endif

    if(tmp == NULL) {
	r->return_val = NULL;
	r->return_size = 0;
    }
    else {
#ifdef _DEBUG_1_
	printf("about to makeCall(%s)\n", fname); fflush(stdout);
#endif
	return_type ret = (*(tmp->fp))(nparams, a);
#ifdef _DEBUG_1_
	printf("makeCall(), returned from call to %s()\n", fname); fflush(stdout);
#endif
	r->return_size = ret.return_size;
	r->return_val = ret.return_val;
    }

    return;
}

void returnResult(int s, return_type *ret) {
#ifdef _DEBUG_1_
    printf("in returnResult()\n"); fflush(stdout);
#endif

    if(ret == NULL || ret->return_size <= 0) {
	int i = 0;
	sendbytes(s, &i, sizeof(int));
	return;
    }

#ifdef _DEBUG_1_
    printf("returnResult(), return_size = %d\n", ret->return_size);
    fflush(stdout);
#endif

    /* else */
    sendbytes(s, (void *)(&(ret->return_size)), sizeof(int));
    sendbytes(s, ret->return_val, ret->return_size);
}

void freeArgs(arg_type *a) {
    arg_type *tmp;
    for(tmp = a; tmp != NULL; tmp = a) {
	a = a->next;
	if(tmp->arg_size > 0) {
	    free(tmp->arg_val);
	}
	free(tmp);
    }
}

void freeRet(return_type r) {
    if(r.return_size <= 0 || r.return_val == NULL) {
	return;
    }
    
    /* else */

    free(r.return_val);
}

void launch_server() {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a;

    memset(&a, 0, sizeof(struct sockaddr_in));
    a.sin_family = AF_INET;
    a.sin_port = 0;
    if((a.sin_addr.s_addr = getPublicIPAddr()) == 0) {
	fprintf(stderr, "Could not get public ip address. Exiting...\n");
	exit(0);
    }

    if(mybind(s, &a) < 0) {
	fprintf(stderr, "mybind() failed. Exiting...\n");
	exit(0);
    }

    printf("%s %u\n", inet_ntoa(a.sin_addr), ntohs(a.sin_port));
    
    if(listen(s, 0) < 0) {
	perror("listen"); exit(0);
    }

    memset(&a, 0, sizeof(struct sockaddr_in));
    socklen_t alen = sizeof(struct sockaddr_in);
    int asock = -1;
    while((asock = accept(s, (struct sockaddr *)&a, &alen)) > 0) {
	/* Single-threaded */

	char *fname;
	int nparams;
	arg_type *a = NULL;
	return_type ret;

	recvCall(asock, &fname, &nparams, &a);

#ifdef _DEBUG_1_
	printf("launch_server(), before makeCall()\n"); fflush(stdout);
#endif

	makeCall(fname, nparams, a, &ret);

#ifdef _DEBUG_1_
	printf("launch_server(), after makeCall()\n"); fflush(stdout);
#endif

	returnResult(asock, &ret);

	free(fname);
	freeArgs(a);
	freeRet(ret);

	shutdown(asock, SHUT_RDWR); close(asock);
	asock = -1;
    }

    /* WARNING -- massive memory, linked list of registered
     * functions! We assume that the process exits at this
     * point, and so do not bother freeing the memory. */
    return;
}
