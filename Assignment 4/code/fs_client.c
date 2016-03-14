#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ece454_fs.h"


void printBuf(char *buf, int size)
{
	/* Should match the output from od -x */
	int i;
	for(i = 0; i < size; )
	{
		if(i%16 == 0)
		{
			printf("%08o ", i);
		}

		int j;
		for(j = 0; j < 16;)
		{
			int k;
			for(k = 0; k < 2; k++)
			{
				if(i+j+(1-k) < size)
				{
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

int main(int argc, char *argv[])
{
	char sep[] ="/";
	if(argc < 4)
	{
		fprintf(stderr, "usage: %s <srv-ip/name> <srv-port> <local dir name>\n", argv[0]);
		exit(1);
	}

	char *dirname = argv[3];
	
	//For mount, we are NOT including the end slash of the directory
	int dirLength = strlen(dirname);
	if(dirLength>0)
	{
		if(dirname[dirLength-1]==sep[0])
		{
			dirname[dirLength-1]='\0';
		}
	}
	
	printf("fsMount(): %d\n", fsMount(argv[1], atoi(argv[2]), dirname));
	FSDIR *fd = fsOpenDir(dirname);
	if(fd == NULL)
	{
		perror("fsOpenDir"); exit(1);
	}

	struct fsDirent *fdent = NULL;
	for(fdent = fsReadDir(fd); fdent != NULL; fdent = fsReadDir(fd))
	{
		printf("\t %s, %d\n", fdent->entName, (int)(fdent->entType));
	}

	if(errno != 0)
	{
		perror("fsReadDir");
	}

	printf("fsCloseDir(): %d\n", fsCloseDir(fd));

	int ff = open("/dev/urandom", 0);
	if(ff < 0)
	{
		perror("open(/dev/urandom)"); exit(1);
	}
	else printf("open(): %d\n", ff);

	char fname[256];
	sprintf(fname, "%s/", dirname);
	if(read(ff, (void *)(fname+strlen(dirname)+1), 10) < 0)
	{
		perror("read(/dev/urandom)"); exit(1);
	}

	int i;
	for(i = 0; i < 10; i++)
	{
		//printf("%d\n", ((unsigned char)(fname[i]))%26);
		fname[i+strlen(dirname)+1] = ((unsigned char)(fname[i+strlen(dirname)+1]))%26 + 'a';
	}
	fname[10+strlen(dirname)+1] = (char)0;
	printf("Filename to write: %s\n", (char *)fname);

	char buf[256];
	if(read(ff, (void *)buf, 256) < 0)
	{
		perror("read(2)"); exit(1);
	}

	printBuf(buf, 256);

	printf("close(): %d\n", close(ff));

	ff = fsOpen(fname, 1);
	if(ff < 0)
	{
		perror("fsOpen(write)"); exit(1);
	}

	if(fsWrite(ff, buf, 256) < 256)
	{
		fprintf(stderr, "fsWrite() wrote fewer than 256\n");
	}

	if(fsClose(ff) < 0)
	{
		perror("fsClose"); exit(1);
	}

	char readbuf[256];
	if((ff = fsOpen(fname, 0)) < 0)
	{
		perror("fsOpen(read)"); exit(1);
	}

	int readcount = -1;

	if((readcount = fsRead(ff, readbuf, 256)) < 256)
	{
		fprintf(stderr, "fsRead() read fewer than 256\n");
	}

	if(memcmp(readbuf, buf, readcount))
	{
		fprintf(stderr, "return buf from fsRead() differs from data written!\n");
	}
	else
	{
		printf("fsread(): return buf identical to data written upto %d bytes.\n", readcount);
	}

	if(fsClose(ff) < 0)
	{
		perror("fsClose"); exit(1);
	}

	printf("fsRemove(%s): %d\n", fname, fsRemove(fname));

	if(fsUnmount(dirname) < 0)
	{
		perror("fsUnmount"); exit(1);
	}

	return 0;
}

