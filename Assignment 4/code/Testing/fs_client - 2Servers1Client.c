#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ece454_fs.h"

int main(int argc, char *argv[]) {
    if (argc < 5) {
        printf("Need: <ip1> <port1> <ip2> <port2> \n");
        exit(1);
    }
    const char *ip = argv[1];
    const int port = atoi(argv[2]);

    const char *ip2 = argv[3];
    const int port2 = atoi(argv[4]);
	
	printf("fsMount1:\n");
    if (fsMount(ip, port, "foo") < 0) {
        perror("fsMount"); exit(1);
    }

	printf("fsMount2:\n");
    if (fsMount(ip, port, "foo") >= 0) {
        printf("ERROR: Should not be able to mount the same folder twice\n");
        exit(1);
    }

	printf("fsMount3:\n");
    if (fsMount(ip2, port2, "foo2") < 0) {
        perror("fsMount"); exit(1);
    }

	printf("open1:\n");
    int fd;
    if ((fd = fsOpen("foo/file1.txt", 0)) < 0) {
        perror("fsOpen"); exit(1);
    }	
	printf("fd: %d\n", fd);

	printf("open2:\n");
    int fd2;
    if ((fd2 = fsOpen("foo2/file2.txt", 0)) < 0) {
        perror("fsOpen"); exit(1);
    }
	printf("fd2: %d\n", fd2);

	printf("open3:\n");
    if (fsOpen("foo/file2.txt", 0) >= 0) {
        printf("ERROR: There should be no file2.txt in server#1\n");
        exit(1);
    }

	printf("open4:\n");
    if (fsOpen("foo2/file1.txt", 0) >= 0) {
        printf("ERROR: There should be no file1.txt in server#2\n");
        exit(1);
    }

	printf("open5:\n");
    if (fsOpen("foo3/file1.txt", 0) >= 0) {
        printf("ERROR: There is no mounted folder called foo3\n");
        exit(1);
    }

	printf("read1:\n");
	printf("fd: %d\n", fd);
	printf("fd2: %d\n", fd2);
    char buf[3001];
    int bytesread;
    int i = 0;
    bytesread = fsRead(fd, (void *)buf, 3000);
	perror("fsRead");
    *((char *) buf + bytesread) = '\0';
	printf("Bytes=%d, String='%s'\n", bytesread, buf);
    if (strncmp(buf, "Hello World1", 12) != 0) {
        printf("ERROR: Read on file1.txt should return 'Hello World1' \n");
        exit(1);
    }

	printf("read2:\n");
    bytesread = fsRead(fd2, (void *)buf, 3000);
    *((char *) buf + bytesread) = '\0';
    if (strncmp(buf, "Hello World2", 12) != 0) {
        printf("ERROR: Read on file2.txt should return 'Hello World2' \n");
        exit(1);
    }

	printf("read3:\n");
    while ((bytesread = fsRead(fd2, (void *)buf, 3000)) > 0) {
        *((char *) buf + bytesread) = '\0';
        printf("%s", (char *) buf);
        i += 1;
    }
    printf("\n");
	
	
	printf("close:\n");
    if (fsClose(fd) < 0) {
        perror("fsClose"); exit(1);
    }
	
	if ((fd = fsOpen("foo/file1.txt", 1)) < 0) {
        perror("fsOpen"); exit(1);
    }	
	printf("write 1:\n");
	//Write to "../server1/file1.txt"
	if(fsWrite(fd,(void *)"Hello Hello", 8)<0)
	{
		printf("write failed:\n");
	}
	
	printf("write 2:\n");
	//Write to "../server1/file1.txt"
	if(fsWrite(fd,(void *)"Hola Hola", 4)<0)
	{
		printf("write2 failed:\n");
	}
	
	
	
	printf("close:\n");
    if (fsClose(fd) < 0) {
        perror("fsClose"); exit(1);
    }

	printf("close2:\n");
    if (fsClose(fd2) < 0) {
        perror("fsClose"); exit(1);
    }
	
	
	// printf("remove:\n");
    // if (fsRemove("foo/file1.txt") < 0) {
        // printf("ERROR: remove file 1 failed\n"); exit(1);
    // }

	// printf("remove2:\n");
    // if (fsRemove("foo") == 0 || fsRemove("foo2") == 0) {
        // printf("ERROR: Should not be able to delete mounted folder\n"); exit(1);
    // }

	printf("openDir:\n");
	FSDIR* tempFsDir=fsOpenDir("foo");
	if(tempFsDir==NULL)
	{
		printf("FSDir is NULL\n");
	}
	printf("readDir:\n");
	struct fsDirent* tempfsDirent=NULL;
	
	do
	{
		tempfsDirent=fsReadDir(tempFsDir);
		if(tempfsDirent!=NULL)
		{
			printf("fsDirent: entName: %s\n", tempfsDirent->entName);
		}
	}while(tempfsDirent!=NULL);
	//printf("fsDirent: entName: %s\n", tempfsDirent->entName);
	
	printf("closeDir:\n");
	if(fsCloseDir(tempFsDir)<0)
	{
		printf("closeDir failed\n");
	}

	

	printf("unmount:\n");
    if (fsUnmount("foo") < 0) {
        perror("fsUnmount"); exit(1);
    }

	printf("unmount2:\n");
    if (fsUnmount("foo2") < 0) {
        perror("fsUnmount"); exit(1);
    }

    printf("All tests passed!\n");
    return 0;
}
