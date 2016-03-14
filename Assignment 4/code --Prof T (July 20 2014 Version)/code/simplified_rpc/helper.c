/* Mahesh V. Tripunitara
   University of Waterloo
   Part of a simplified RPC implementation
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include "ece454rpc_types.h"

#if 0
#define _DEBUG_1_
#endif

uint32_t getPublicIPAddr() {
    struct ifaddrs *ifa;

    if(getifaddrs(&ifa) < 0) {
	perror("getifaddrs"); exit(0);
    }

    struct ifaddrs *c;
    for(c = ifa; c != NULL; c = c->ifa_next) {
	if(c->ifa_addr == NULL) continue;
	if(c->ifa_addr->sa_family == AF_INET) {
	    struct sockaddr_in a;

	    memcpy(&a, (c->ifa_addr), sizeof(struct sockaddr_in));
	    char *as = inet_ntoa(a.sin_addr);
	    //printf("%s\n", as);
	    
	    int apart;
	    sscanf(as, "%d", &apart);
	    if(apart > 0 && apart != 127) {
		freeifaddrs(ifa);
		return (a.sin_addr.s_addr);
	    }
	}
    }

    freeifaddrs(ifa);
    return 0;
}

#if 0
void printBuf(char *buf, int size) {
    /* Should match the output from od -x */
    int i;
    for(i = 0; i < size; ) {
	if(i%16 == 0) {
	    printf("%08o ", i);
	}

	int j;
	for(j = 0; j < 16;) {
	    int k;
	    for(k = 0; k < 2; k++) {
		if(i+j+(1-k) < size) {
		    printf("%02x", (unsigned char)(buf[i+j+(1-k)]));
		}
	    }

	    printf(" ");
	    j += k;
	}

	printf("\n");
	i += j;
    }
}
#endif

void recvbytes(int s, void *buf, ssize_t count) {
    /* Recv until we hit count bytes */
    int stillToRecv;
    for(stillToRecv = count; stillToRecv > 0; ) {
	ssize_t recvSize =
	    recv(s, (void *)(((unsigned char *)buf) + count - stillToRecv), stillToRecv, 0);

	if(recvSize <= 0) {
	    perror("recv"); exit(1);
	}

#ifdef _DEBUG_1_
	printf("recvbytes():\n");
	printBuf((char *)buf + count - stillToRecv, recvSize);
	fflush(stdout);
#endif

	stillToRecv -= recvSize;
    }
}

void sendbytes(int s, void *buf, ssize_t count) {
    int stilltosend;
    for(stilltosend = count; stilltosend > 0; ) {
	ssize_t sentsize =
	    send(s, (void *)(((unsigned char *)buf) + count - stilltosend), stilltosend, MSG_NOSIGNAL);
	if(sentsize < 0) {
	    perror("send"); exit(1);
	}

#ifdef _DEBUG_1_
	printf("sendbytes():\n");
	printBuf((char *)buf + count - stilltosend, sentsize);
	fflush(stdout);
#endif
	stilltosend -= sentsize;
    }
}
