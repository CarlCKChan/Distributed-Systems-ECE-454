#include "ece454rpc_types.h"
#include "server_stub.c"
#include "fs_server.c"
#include <sys/stat.h>
#include <unistd.h>



void usage(char *pname) {
    fprintf(stderr, "usage: %s <directory to serve>\n", pname);
}

//Register all the procedures in fs_server
int main(int argc, char *argv[])
{
	char sep[] ="/";
	/*********************************************************************************
	 *							PARAMETER CHECKING
	 *********************************************************************************/
	 // Number of parameters
	if(argc < 2) {
		usage(argv[0]); return 1;
    }

	// Check if passed value is a directory
    struct stat buf;
    if(stat(argv[1], &buf) < 0) {
		perror("stat");
		usage(argv[0]);
		return 1;
    }
	
	//TODO: slash check at the end
	char dirname[ECE454_MAX_DIR_STR_SIZE];
	strcpy(dirname, argv[1]);
	int dirLength = strlen(dirname);
	if(dirLength>0)
	{
		if(dirname[dirLength-1]!=sep[0])
		{
			dirname[dirLength]=sep[0];
			dirname[dirLength+1]='\0';
		}
	}
	strcpy(serverRootFolder, dirname);

    if(!S_ISDIR(buf.st_mode)) {
		fprintf(stderr, "error: %s does not appear to be a directory.\n", argv[1]);
		usage(argv[0]);
		return 1;
    }

	/*********************************************************************************
	 *									RPC
	 *********************************************************************************/
	register_procedure("mount", 0, fsServerMount);
    register_procedure("unmount", 0, fsServerUnmount);
    register_procedure("openDir", 1, fsServerOpenDir);
    register_procedure("closeDir", 1, fsServerCloseDir);
    register_procedure("readDir", 1, fsServerReadDir);
    register_procedure("open", 4, fsServerOpen);
    register_procedure("close", 3, fsServerClose);
    register_procedure("read", 2, fsServerRead);
    register_procedure("write", 3, fsServerWrite);
    register_procedure("remove", 3, fsServerRemove);
	launch_server();
	return 0;
}
