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
#include <errno.h>
#include <ifaddrs.h>			// To find own IP
#include <netinet/in.h>			// To find own IP
#include <netinet/ip.h>			// To find own IP
#include <sys/socket.h>			// To find own IP
#include <arpa/inet.h>			// To find own IP
#include <unistd.h>

#include "ece454rpc_types.h"

#include "ece454_fs.h"
#include "fsOtherIncludes.h"
#include "fs_RPCDataTypes.h"

#define IS_FD_TRUE 1
#define IS_FD_FALSE 0

#define RETRY_LOOP_WAIT 1		// Number of seconds to wait if need to rety

char sep[] ="/";

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

struct fdIPMappingNode
{
	bool IsFD;
	int value;
	FSDIR* pointer;
	char serverIP[100];
	int serverPort;
};

struct fdIPMappingList
{
	struct fdIPMappingNode data;
	struct fdIPMappingList* next;
};
struct fdIPMappingList* fdIPMappingListHead=NULL;

void pushFDIPMapping(struct fdIPMappingNode recordToPush)
{
	struct fdIPMappingList* temp;
	temp=(struct fdIPMappingList *)malloc(sizeof(struct fdIPMappingList));
	temp->data=recordToPush;
	temp->next=fdIPMappingListHead;
	fdIPMappingListHead=temp;
}

struct fdIPMappingList* SearchFDIPMappingList(bool IsFD, int value, FSDIR* pointer)
{
	struct fdIPMappingList* result=NULL;
	struct fdIPMappingList* current = fdIPMappingListHead;
	struct fdIPMappingList* previous = current;
	//Empty List with no Head
	if(current==NULL)
	{
		return NULL;
	}
	while(current!=NULL)
	{
		//Found what we are looking for
		if(current->data.IsFD==IsFD&&current->data.value==value)
		{
			result=current;
			return result;
		}
		previous=current;
		current=current->next;
	}
	return result;
}

void removeFDIPMapping(bool IsFD, int value)
{
	struct fdIPMappingList* current = fdIPMappingListHead;
	struct fdIPMappingList* previous = current;
	//Empty List with no Head
	if(current==NULL)
	{
		return;
	}
	while(current!=NULL)
	{
		//Found what we are looking for
		if(current->data.IsFD==IsFD&&current->data.value==value)
		{
			previous->next=current->next;
			free(current);
			return;
		}
		previous=current;
		current=current->next;
	}
}

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
bool clientSideIdSet = 0;

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
			clientSideIdSet = 1;

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
		newNode.port=srvPort;
		strcpy(newNode.localFolderName, localFolderName);

		//For mount, we are NOT including the end slash of the directory
		int dirLength = strlen(newNode.localFolderName);
		if((dirLength > 0) && (newNode.localFolderName[dirLength-1]==sep[0]))
		{
			newNode.localFolderName[dirLength-1]='\0';
		}

		pushMappingNode(newNode);

		return 0;
	}
	else
	{
		// Set errno on error
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
	if((dirLength > 0) && (tempStr[dirLength-1]==sep[0]))
	{
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
	// Find the server this folder is in (IP and Port).  Also get the relative path from the root.
	struct mappingNode* mySrvPntr = RootSearcher(folderName);
	if( mySrvPntr == NULL )
	{
		errno = EACCES;
		return NULL;
	}

	int relPathIndex = searchCommonPath(mySrvPntr->data.localFolderName, folderName);
	const char* relPathName = (char*)&folderName[relPathIndex];

	// The IP and port of the server
	char* mySrvIP = mySrvPntr->data.serverIP;
	int mySrvPort = mySrvPntr->data.port;

	//struct stat sbuf;
	return_type ans;
	ans = make_remote_call(mySrvIP, mySrvPort, "openDir", 1, strlen(relPathName)+1, (void*)relPathName);
	struct sRetDir myDirPntr;

	// If return is correct size, get the returned structure.  Otherwise, there were too many parameters to registered function
	if( ans.return_size == sizeof(myDirPntr) )
	{
		myDirPntr = *(struct sRetDir*)(ans.return_val);
	}
	else 
	{
		errno = E2BIG;
		return NULL;
	}

	// Set errno if the open dir failed
	if( myDirPntr.val == NULL )
	{
		errno = myDirPntr.error;
	}
	else
	{
		struct fdIPMappingNode recordToPush;
		recordToPush.IsFD = IS_FD_FALSE;
		recordToPush.value = (int)(myDirPntr.val);
		strcpy(recordToPush.serverIP, mySrvIP);
		recordToPush.serverPort = mySrvPort;
		pushFDIPMapping(recordToPush);
	}

	return(myDirPntr.val);
}

/*
* TODO : Test this function call
*/
int fsCloseDir(FSDIR *folder)
{
	// Find the server this folder is in (IP and Port).  Also get the relative path from the root.
	struct fdIPMappingList* mySrvPntr = SearchFDIPMappingList(IS_FD_FALSE, (int)folder);
	if( mySrvPntr == NULL )
	{
		errno = EACCES;
		return -1;
	}

	// The IP and port of the server
	char* mySrvIP = mySrvPntr->data.serverIP;
	int mySrvPort = mySrvPntr->data.serverPort;

	// Do RPC to server
	return_type ans;
	ans = make_remote_call(mySrvIP, mySrvPort, "closeDir", 1, sizeof(FSDIR*), (void*)&folder);
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
	if( myRetInt.val == -1 ) {
		errno = myRetInt.error;
	} else {
		removeFDIPMapping(IS_FD_FALSE, folder);
	}

	return(myRetInt.val);
}


/*
* TODO : Test this function call
*/
struct fsDirent *fsReadDir(FSDIR *folder)
{
	//const int initErrno = errno;

	// Find the server this folder is in (IP and Port).  Also get the relative path from the root.
	struct fdIPMappingList* mySrvPntr = SearchFDIPMappingList(IS_FD_FALSE, (int)folder);
	if( mySrvPntr == NULL )
	{
		errno = EACCES;
		return NULL;
	}

	// The IP and port of the server
	char* mySrvIP = mySrvPntr->data.serverIP;
	int mySrvPort = mySrvPntr->data.serverPort;

	// Do RPC to server
	return_type ans;
	ans = make_remote_call(mySrvIP, mySrvPort, "readDir", 1, sizeof(FSDIR*), (void*)&folder);
	struct sRetDent myDent;

	// If return is correct size, get the returned structure.  Otherwise, there were too many parameters to registered function
	if( ans.return_size == sizeof(myDent) )
	{
		myDent = *(struct sRetDent*)(ans.return_val);
	}
	else
	{
		errno = E2BIG;
		return NULL;
	}

	// Set errno on error
	if(myDent.dirPoint == NULL) {
		// if(myDent.error == initErrno) errno = 0;
		errno = myDent.error;
		return NULL;
	}

	dent.entType = myDent.val.entType;
	memcpy(&(dent.entName), &(myDent.val.entName), 256);
	return &dent;
}


/*
* TODO : Test this function call
*/
int fsOpen(const char *fname, int mode)
{
	// Set the client side ID if not already set
	if( clientSideIdSet == 0 ) { populateOwnIpAndPid(); }

	// Find the server this folder is in (IP and Port).  Also get the relative path from the root.
	struct mappingNode* mySrvPntr = RootSearcher(fname);
	if( mySrvPntr == NULL ) {
		errno = EACCES;
		return -1;
	}

	int relPathIndex = searchCommonPath(mySrvPntr->data.localFolderName, fname);
	char* relPathName = &fname[relPathIndex];

	// The IP and port of the server
	char* mySrvIP = mySrvPntr->data.serverIP;
	int mySrvPort = mySrvPntr->data.port;

	//struct stat sbuf;
	return_type ans;
	struct sRetInt myInt;

	// Keep trying to open until get an actual answer
	do
	{
		ans = make_remote_call(mySrvIP, mySrvPort, "open", 4, 
			strlen(clientSideIp)+1, (void*)clientSideIp,		// Client-side IP
			sizeof(clientSidePid), (void*)&clientSidePid,
			strlen(relPathName)+1, (void*)relPathName,
			sizeof(mode), mode
			);

		// If return is correct size, get the returned structure.  Otherwise, there were too many parameters to registered function
		if( ans.return_size == sizeof(myInt) )
		{
			myInt = *(struct sRetInt*)(ans.return_val);
			if( myInt.val == RPC_RETRY_CODE ) { sleep(RETRY_LOOP_WAIT); }
		} 
		else 
		{
			errno = E2BIG;
			return -1;
		}
	} while(myInt.val == RPC_RETRY_CODE);

	// Set errno on error
	if(myInt.val == -1)
	{
		errno = myInt.error;
	} 
	else 
	{
		struct fdIPMappingNode recordToPush;
		recordToPush.IsFD = IS_FD_TRUE;
		recordToPush.value = myInt.val;
		strcpy(recordToPush.serverIP, mySrvIP);
		recordToPush.serverPort = mySrvPort;
		pushFDIPMapping(recordToPush);
	}

	return( myInt.val );
}

/*
* TODO : Test this function call
*/
int fsClose(int fd)
{
	// Set the client side ID if not already set
	if( clientSideIdSet == 0 ) { populateOwnIpAndPid(); }

	// Find the server this folder is in (IP and Port).  Also get the relative path from the root.
	struct fdIPMappingList* mySrvPntr = SearchFDIPMappingList(IS_FD_TRUE, fd);
	if( mySrvPntr == NULL ) 
	{
		errno = EBADF;
		return -1;
	}

	// The IP and port of the server
	char* mySrvIP = mySrvPntr->data.serverIP;
	int mySrvPort = mySrvPntr->data.serverPort;

	// Do RPC to server
	return_type ans;
	ans = make_remote_call(mySrvIP, mySrvPort, "close", 3, 
		strlen(clientSideIp)+1, (void*)clientSideIp,		// Client-side IP
		sizeof(clientSidePid), (void*)&clientSidePid,
		sizeof(fd), (void*)&fd);
	struct sRetInt myInt;

	// If return is correct size, get the returned structure.  Otherwise, there were too many parameters to registered function
	if( ans.return_size == sizeof(myInt) ) 
	{
		myInt = *(struct sRetInt*)(ans.return_val);
	} 
	else 
	{
		errno = E2BIG;
		return -1;
	}

	// Set errno on error
	if(myInt.val == -1) 
	{
		errno = myInt.error;
	} 
	else 
	{
		removeFDIPMapping(IS_FD_TRUE, fd);
	}

	return(myInt.val);
}

/*
* TODO : Test this function call
*/
int fsRead(int fd, void *buf, const unsigned int count)
{
	// Find the server this folder is in (IP and Port).  Also get the relative path from the root.
	struct fdIPMappingList* mySrvPntr = SearchFDIPMappingList(IS_FD_TRUE, fd);
	if( mySrvPntr == NULL ) 
	{
		errno = EBADF;
		return -1;
	}

	// The IP and port of the server
	char* mySrvIP = mySrvPntr->data.serverIP;
	int mySrvPort = mySrvPntr->data.serverPort;

	// Do RPC to server
	return_type ans;
	ans = make_remote_call(mySrvIP, mySrvPort, "write", 2,
		sizeof(fd), (void*)&fd,
		sizeof(count), (void*)&count);
	int myError;
	int myBytesRead;
	void* myReadBuf;
	char* myFullRetAsChar;			// The return as an easily indexable char array

	// If return is correct size, get the returned data.  Otherwise, there were too many parameters to registered function
	if( ans.return_size >= READ_BUF_DATA_IDX ) 
	{

		// Parse out the values return by the read operation and set the passed read buffer
		myFullRetAsChar = (char*)(ans.return_val);
		myError = *(int*)( &myFullRetAsChar[READ_BUF_ERRNO_IDX] );
		myBytesRead = *(int*)( &myFullRetAsChar[READ_BUF_NUMBYTES_IDX] );
		myReadBuf = (void*)( &myFullRetAsChar[READ_BUF_DATA_IDX] );
		memcpy(buf, myReadBuf, (size_t)myBytesRead);

	} 
	else 
	{
		errno = E2BIG;
		return -1;
	}

	// Set errno on error
	if(myBytesRead == -1) 
	{
		errno = myError;
	}

	return(myBytesRead);
}


/*
* TODO : Test this function call
*/
int fsWrite(int fd, const void *buf, const unsigned int count)
{
	// Find the server this folder is in (IP and Port).  Also get the relative path from the root.
	struct fdIPMappingList* mySrvPntr = SearchFDIPMappingList(IS_FD_TRUE, fd);
	if( mySrvPntr == NULL ) 
	{
		errno = EBADF;
		return -1;
	}

	// The IP and port of the server
	char* mySrvIP = mySrvPntr->data.serverIP;
	int mySrvPort = mySrvPntr->data.serverPort;

	// Do RPC to server
	return_type ans;
	ans = make_remote_call(mySrvIP, mySrvPort, "write", 3,
		sizeof(fd), (void*)&fd,
		count, buf,
		sizeof(count), (void*)&count);
	struct sRetInt myInt;

	// If return is correct size, get the returned structure.  Otherwise, there were too many parameters to registered function
	if( ans.return_size == sizeof(myInt) ) 
	{
		myInt = *(struct sRetInt*)(ans.return_val);
	} 
	else 
	{
		errno = E2BIG;
		return -1;
	}

	// Set errno on error
	if(myInt.val == -1) 
	{
		errno = myInt.error;
	}

	return(myInt.val); 
}


/*
* TODO : Test this function call
*/
int fsRemove(const char *name)
{
	// Set the client side ID if not already set
	if( clientSideIdSet == 0 ) { populateOwnIpAndPid(); }

	// Find the server this folder is in (IP and Port).  Also get the relative path from the root.
	struct mappingNode* mySrvPntr = RootSearcher(name);
	if( mySrvPntr == NULL ) {
		errno = EACCES;
		return -1;
	}

	int relPathIndex = searchCommonPath(mySrvPntr->data.localFolderName, name);
	char* relPathName = &name[relPathIndex];

	// The IP and port of the server
	char* mySrvIP = mySrvPntr->data.serverIP;
	int mySrvPort = mySrvPntr->data.port;

	//struct stat sbuf;
	return_type ans;
	struct sRetInt myInt;

	// Keep trying to open until get an actual answer
	do 
	{
		ans = make_remote_call(mySrvIP, mySrvPort, "remove", 3, 
			strlen(clientSideIp)+1, (void*)clientSideIp,		// Client-side IP
			sizeof(clientSidePid), (void*)&clientSidePid,
			strlen(relPathName)+1, (void*)relPathName
			);

		// If return is correct size, get the returned structure.  Otherwise, there were too many parameters to registered function
		if( ans.return_size == sizeof(myInt) ) 
		{
			myInt = *(struct sRetInt*)(ans.return_val);
			if( myInt.val == RPC_RETRY_CODE ) { sleep(RETRY_LOOP_WAIT); }
		} 
		else 
		{
			errno = E2BIG;
			return -1;
		}
	} while( myInt.val == RPC_RETRY_CODE );

	// Set errno on error
	if(myInt.val == -1)
	{
		errno = myInt.error;
	}

	return(myInt.val);
}
