/* 
* Mahesh V. Tripunitara
* University of Waterloo
* A dummy implementation of the functions for the remote file
* system. This file just implements those functions as thin
* wrappers around invocations to the local filesystem. In this
* way, this dummy implementation helps clarify the semantics
* of those functions. Look up the man pages for the calls
* such as opendir() and read() that are made here.
*/
#include "ece454_fs.h"
#include <string.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdlib.h>
#include "fsOtherIncludes.h"
#include <errno.h>
#include <ifaddrs.h>			// To find own IP
#include <netinet/in.h>			// To find own IP
#include <netinet/ip.h>			// To find own IP
#include <sys/socket.h>			// To find own IP
#include <arpa/inet.h>			// To find own IP

#include "ece454rpc_types.h"

#include "ece454_fs.h"

struct mountMapping
{
	char serverIP[100];
	int port;
	char localFolderName[100];
};

struct mappingNode
{
	struct mountMapping data;
	struct mappingNode* next;
};
struct mappingNode* mappingHead = NULL;

void pushMappingNode(struct mountMapping recordToPush)
{
	struct mappingNode *temp;
	temp= (struct mappingNode *)malloc(sizeof(struct mappingNode));
	temp->data=recordToPush;
	temp->next=mappingHead;
	mappingHead = temp;
}
//TODO: Fix to include "/"
int searchCommonPath(const char* root, const char* fullPath)
{
	int i;
	int result=-1;
	int rootLength;
	char sep[] ="/";

	//Returns a -1 if the root is longer than the full path
	if(strlen(root)>strlen(fullPath))
	{
		return -1;
	}
	rootLength=strlen(root);
	
	if(strcmp(root, fullPath)==0)
	{
		return rootLength;
	}

	for(i=0; i<rootLength;i++)
	{
		if(root[i]==fullPath[i])
		{
			continue;
		}
		else
		{
			return -1;
		}
	}
	result=i;
	
	//Last character of the root is a "/"
	if(strcmp(&root[rootLength-1], sep)==0)
	{
		return result;
	}
	else
	{
		//need to check whether the fullPath is a slash or not
		printf("InFunc: %c\n", fullPath[rootLength]);
		if(fullPath[rootLength] != sep[0])
		{
			return -1;
		}
		else
		{
			return result+1;
		}
	}

	return result;
}
//TODO: Search for root:
struct mappingNode* RootSearcher(const char* fullPath)
{
	struct mappingNode *result=NULL;
	struct mappingNode* current = mappingHead;
	struct mappingNode* previous = current;
	//Empty List with no Head
	if(current==NULL)
	{
		return NULL;
	}
	while(current!=NULL)
	{
		//Found what we are looking for
		if(searchCommonPath(current->data.localFolderName,fullPath) > 0)
		{
			result=current;
			return result;
		}
		previous=current;
		current=current->next;
	}
	return result;
}

//TODO: Remove by LocalFolderName (remove all of them)
void RemoveFolderMappingByName(const char* folderToRemove)
{
	struct mappingNode* current = mappingHead;
	struct mappingNode* previous = current;
	//Empty List with no Head
	if(current==NULL)
	{
		return;
	}
	while(current!=NULL)
	{
		//Found what we are looking for
		if(strcmp(current->data.localFolderName,folderToRemove) == 0)
		{
			//the thing we are looking for is the first node
			if(previous == current)
			{
				mappingHead=current->next;
				free(current);
				//return;
			}
			else
			{
				previous->next=current->next;
				free(current);
				//return;
			}
		}
		previous=current;
		current=current->next;
	}
}


/************************************************************************************************
 *						Helper functions related to identifying this process
 ************************************************************************************************/
char clientSideIp[100];
pid_t clientSidePid;
 
void populateOwnIpAndPid() {

	//Get a list of IP addresses of the server machine's network interfaces
	struct ifaddrs *server_interface, *tempPointer = NULL;
	getifaddrs(&server_interface);
	tempPointer=server_interface;
	
	// Get the IPv4 addresses of the first Ethernet interface found, along with the process ID
	for(tempPointer = server_interface; tempPointer!=NULL; tempPointer=tempPointer->ifa_next)
	{
		if ( (tempPointer->ifa_addr->sa_family==PF_INET) && (strcmp((*tempPointer).ifa_name,"eth0") == 0) )
		{
			char * addr=inet_ntoa(((struct sockaddr_in *)(tempPointer->ifa_addr))->sin_addr);
			
			strcpy(clientSideIp, addr);
			clientSidePid = getpid();
			
			break;
		}
	}
	
	// Free the memory associated with the interface information
	freeifaddrs(server_interface);

}

/*********************************************************************************************
 *                             CLIENT-SIDE FS FUNCTIONS
 *********************************************************************************************/


struct fsDirent dent;


/*
 * TODO : Test this function call
 */
int fsMount(const char *srvIpOrDomName, const unsigned int srvPort, const char *localFolderName)
{
	//struct stat sbuf;
	return_type ans;
	ans = make_remote_call(srvIpOrDomName, srvPort, "mount", 0);
	struct sRetAck myAck;
	
	// If return is correct size, get the returned structure.  Otherwise, there were too many parameters to registered function
	if( ans.return_size == sizeof(myAck) )
	{
		myAck = *(struct sRetAck*)(ans.return_val);
	}
	else 
	{
		errno = E2BIG;
		return -1;
	}
	
	if( myAck.val == GOOD ) 
	{
		struct mountMapping newNode;
		strcpy(newNode.serverIP, srvIpOrDomName);
		newNode.port=srvPort
		strcpy(newNode.localFolderName, localFolderName);
		
		//For mount, we are NOT including the end slash of the directory
		int dirLength = strlen(newNode.localFolderName);
		if((dirLength > 0) && (newNode.localFolderName[dirLength-1]==sep[0])) {
			newNode.localFolderName[dirLength-1]='\0';
		}
		
		pushMappingNode(newNode);
		
		return 0;
	} else {
		errno = myAck.error;
		return -1;
	}
	
	//return(stat(localFolderName, &sbuf));
}

/*
 * TODO : Test this function call
 */
int fsUnmount(const char *localFolderName)
{
	// Make a local copy of file name
	int dirLength = strlen(localFolderName);
	char* tempStr = (char*)malloc(dirLength + 1);
	strcpy(tempStr, localFolderName);
	
	// For unmount, we are NOT including the end slash of the directory when matching, since the
	// roots are recorded without them.
	if((dirLength > 0) && (tempStr[dirLength-1]==sep[0])) {
		
		tempStr[dirLength-1]='\0';
	}
	
	// Remove the mapping for the local folder and 
	RemoveFolderMappingByName(tempStr);
	free(tempStr);
	
	return 0;
}


/*
 * TODO : Test this function call
 */
FSDIR* fsOpenDir(const char *folderName)
{
	//struct stat sbuf;
	return_type ans;
	ans = make_remote_call(srvIpOrDomName, srvPort, "openDir", 1, strlen(folderName)+1, (void*)folderName);
	struct sRetDir myDirPntr;
	
	// If return is correct size, get the returned structure.  Otherwise, there were too many parameters to registered function
	if( ans.return_size == sizeof(myDirPntr) )
	{
		myDirPntr = *(struct sRetDir*)(ans.return_val);
	}
	else 
	{
		errno = E2BIG;
		return -1;
	}
	
	// Set errno if the open dir failed
	if( myDirPntr.val == NULL )
	{
		errno = myDirPntr.error;
	}
	
	return(myDirPntr.val);
}

/*
 * TODO : Test this function call
 */
int fsCloseDir(FSDIR *folder)
{
	return_type ans;
	ans = make_remote_call(srvIpOrDomName, srvPort, "closeDir", 1, sizeof(FSDIR*), (void*)&folder;
	struct sRetInt myRetInt;
	
	// If return is correct size, get the returned structure.  Otherwise, there were too many parameters to registered function
	if( ans.return_size == sizeof(myRetInt) )
	{
		myRetInt = *(struct sRetInt*)(ans.return_val);
	}
	else 
	{
		errno = E2BIG;
		return -1;
	}
	
	// Set errno if the close dir failed
	if( myRetInt.val == -1 )
	{
		errno = myRetInt.error;
	}
	
	return(myRetInt.val);
}


/*
 * TODO : Test this function call
 */
struct fsDirent *fsReadDir(FSDIR *folder) {
	//const int initErrno = errno;
	
	return_type ans;
	ans = make_remote_call(srvIpOrDomName, srvPort, "readDir", 1, sizeof(FSDIR*), (void*)&folder);
	struct sRetDent myDent;
	
	// If return is correct size, get the returned structure.  Otherwise, there were too many parameters to registered function
	if( ans.return_size == sizeof(myDent) ) {
		myDent = *(struct sRetDent*)(ans.return_val);
	} else {
		errno = E2BIG;
		return -1;
	}
	
	if(myDent.dirPoint == NULL) {
		// if(myDent.error == initErrno) errno = 0;
		errno = myDent.error;
		return NULL;
	}
	
	dent.entType = myDent.val.entType;
	memcpy(&(dent.entName), &(myDent.val.entName), 256);
	return &dent;
}




int fsOpen(const char *fname, int mode)
{
	return_type ans;
	ans = make_remote_call(srvIpOrDomName, srvPort, "closeDir", 1, sizeof(FSDIR*), (void*)&folder);
	struct sRetInt myInt;
	
	// If return is correct size, get the returned structure.  Otherwise, there were too many parameters to registered function
	if( ans.return_size == sizeof(myInt) ) {
		myInt = *(struct sRetInt*)(ans.return_val);
	} else {
		errno = E2BIG;
		return -1;
	}
	
	int flags = -1;

	if(mode == 0)
	{
		flags = O_RDONLY;
	}
	else if(mode == 1)
	{
		flags = O_WRONLY | O_CREAT;
	}

	return(open(fname, flags, S_IRWXU));
}

//Done (need to queue, but the function itself is done)
//Returns 0 on success, -1 on failure
int fsClose(int fd)
{
	return(close(fd));
}

//Done (need to queue, but the function itself is done)
int fsRead(int fd, void *buf, const unsigned int count)
{
	return(read(fd, buf, (size_t)count));
}

//Done (need to queue, but the function itself is done)
int fsWrite(int fd, const void *buf, const unsigned int count)
{
	return(write(fd, buf, (size_t)count)); 
}

//Done (need to queue, but the function itself is done)
int fsRemove(const char *name)
{
	return(remove(name));
}
