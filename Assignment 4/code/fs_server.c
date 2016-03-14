#include "ece454_fs.h"
#include "fsOtherIncludes.h"
#include "ece454rpc_types.h"
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include "fs_RPCDataTypes.h"

#define ECE454_MAX_DIR_STR_SIZE 256		// Maximum size of a directory name (relative or absolute)
#define ECE454_OPEN_MODE_READ 0			// Mode value when opening with read mode
#define ECE454_OPEN_MODE_WRITE 1		// Mode value when opening with write mode

#if 0
#define _DEBUG_1_
#endif

/************************************************************************************************
*								Global Variables for Returning Values
************************************************************************************************/
return_type ret;			// Return type that all RPC returns use

struct sRetAck serverAck;				// Acknowledgement from server for certain RPC calls

struct sRetDir retDirPntr;				// Stores pointer to FSDIR so RPC can send the pointer itself.  Does not need to persist after that.

struct sRetInt retDirIntVal;				// Stores an int value so RPC can send it.

struct sRetDent retDent;					// File directory entity to return

char* retReadBuff = NULL;	// To point to memory location currently used to return read buffer
// From front: 4 bytes errno, 4 bytes read count, rest is content

/************************************************************************************************
*							Global Variables for Server Configuration
************************************************************************************************/
char serverRootFolder[ECE454_MAX_DIR_STR_SIZE];	// Name of root folder.
// TODO : Move this to other file?
char fileSeparator[] = "/";						// File separator on this machine



// A type describing actions the client can request that involve getting a lock (does not include stuff like read and write).
typedef enum {REMOVE, OPEN, CLOSE} fsLockOpCode_t;

/*
* The requests from clients which can get queued.  The only ones which get queued are the ones that manipulate the lock on
* a file.  This is as these need to wait if the file is locked by another process, while operations like read and write can
* just fail immediately if that is the case.
*/ 
//Level 2
struct fsLockOpRequest
{
	char clientIP[100];			// IP of client machine
	pid_t clientPID;			// PID of client process on client machine
	fsLockOpCode_t mode;		// The operation being requested by the client (ex. remove file, open file, etc.)
};


//TODO: Level 2 Linked List implementation
struct level2Node                                                
{
	struct fsLockOpRequest data;	// will store information
	struct level2Node *next;				// the reference to the next level2Node
};


// Represents the data the server needs to know about one file
//Level 1
struct fsFileRecord
{
	char fileName[255] ;			// Relative path of file from the server's base file directory
	int fileDesciptor;			// File descriptor, if file is open
	bool hasLock;				// Whether a client currently has a lock on this file
	char clientIP[100];			// IP of client machine
	pid_t clientPID;			// PID of client process on client machine
	struct level2Node* level2head;	// A queue of requests that manipulate the file lock
};


void populateFileRecord( struct fsFileRecord* myRec, char *fileName, int fileDesciptor, bool hasLock, char clientIP[100], pid_t clientPID, struct level2Node* level2head )
{
	strcpy( myRec->fileName, fileName );
	myRec->fileDesciptor = fileDesciptor;
	myRec->hasLock = hasLock;
	strcpy( myRec->clientIP, clientIP );
	myRec->clientPID = clientPID;
	myRec->level2head = level2head;
}

//Level 1 Node implementation

struct level1Node
{
	struct fsFileRecord data;
	struct level1Node *next;
};
struct level1Node* level1head = NULL;

void pushLevel1Node(struct fsFileRecord recordToPush)
{
	struct level1Node *temp;
	temp= (struct level1Node *)malloc(sizeof(struct level1Node));
	temp->data=recordToPush;
	temp->next=level1head;
	level1head = temp;
}

struct level1Node* searchLevel1ByFileName(char* stringToSearchFor)
{
	struct level1Node* current = level1head;
	while(current!=NULL)
	{
		if(strcmp(current->data.fileName,stringToSearchFor)==0)
		{
			return current;
		}
		current=current->next;
	}
	return NULL;
}

struct level1Node* searchLevel1ByFileDescriptor(int fdToSearchFor)
{
	struct level1Node* current = level1head;
#ifdef _DEBUG_1_
	printf("\n\nsearchLevel1ByFileDescriptor()\n"); fflush(stdout);
#endif
	while(current!=NULL)
	{
		if(current->data.fileDesciptor==fdToSearchFor)
		{
			return current;
		}
		current=current->next;
	}
	return NULL;
}

void removeLevel1Node(char* fileNameToDelete)
{
	struct level1Node* current = level1head;
	struct level1Node* previous = current;
	//Empty List with no Head
	if(current==NULL)
	{
		return;
	}
	while(current!=NULL)
	{
		//Found what we are looking for
		if(strcmp(current->data.fileName,fileNameToDelete) == 0)
		{
			//the thing we are looking for is the first node
			if(previous == current)
			{
				level1head=current->next;
				free(current);
				return;
			}
			else
			{
				previous->next=current->next;
				free(current);
				return;
			}
		}
		previous=current;
		current=current->next;
	}
}

//End of Level 1 node implementation

//Need to append to the front of the list, returning new head
struct level2Node* pushLevel2Node(struct level2Node* level2head,struct fsLockOpRequest input)
{
	struct level2Node *temp;
	temp= (struct level2Node *)malloc(sizeof(struct level2Node));
	temp->data=input;
	temp->next=level2head;
	return temp;
}

//Delete from the back of the list, returning new head
struct level2Node* popLevel2NodeList(struct level2Node* level2Head)
{
	struct level2Node *previous, *current;
	struct level2Node* newLevel2Head;
	newLevel2Head=NULL;
	current = level2Head;
	previous=current;
	//Empty List
	if(current==NULL)
	{
		return NULL;
	}
	while(current->next!=NULL)
	{
		previous=current;
		current=current->next;
	}
	//Only 1 element in the list
	if(previous==current)
	{
		free(previous);
		return NULL;
	}
	previous->next=NULL;
	free(current);
	newLevel2Head=level2Head;
	return newLevel2Head;
}

/*
* Returns true if found a matching op request in the Level 2 queue
*/
bool searchLevel2(struct level2Node* level2Head, char* clientIP, pid_t clientPID, fsLockOpCode_t mode)
{
	struct level2Node* current = level2Head;
	while(current!=NULL)
	{
		if((strcmp(current->data.clientIP, clientIP)==0) 
			&& current->data.clientPID==clientPID
			&& current->data.mode==mode)
		{
			return true;
		}
		current=current->next;
	}
	return false;
}

// Returns the next thing that will be popped of the Level 2 queue.  DOES NOT actually alter the data.
struct fsLockOpRequest findNextRequestInLevel2(struct level2Node* level2Head)
{
	struct level2Node* current = level2Head;
	struct fsLockOpRequest result;
	if(current==NULL)
	{
		//Returning NULL
		strcpy(result.clientIP,"NULL");
		result.clientPID = 0;
		result.mode=CLOSE;
	}
	else
	{
		while(current->next!=NULL)
		{
			current=current->next;
		}
		//current is now the last element of the LinkedList
		strcpy(result.clientIP, current->data.clientIP);
		result.clientPID = current->data.clientPID;
		result.mode=current->data.mode;
	}
	return result;
}

//End Level 2 Linked List




// A collection of all the data about all the files currently in use by the server

/*
* Params: Nothing
* Description: Only need to tell the user that the server is running (available to mount)
*/
return_type fsServerErrorReturn()
{
	ret.return_val = NULL;
	ret.return_size = 0;
	return ret;
}


/*
* Params: Nothing
* Description: Only need to tell the user that the server is running (available to mount)
*/
return_type fsServerMount(const int nparams, arg_type * a)
{
	serverAck.val = GOOD;

	ret.return_val = (void*)&serverAck;
	ret.return_size = sizeof(serverAck);

	return(ret);
}

/*
* Params: Nothing (?).  Do we need to close everything before unmounting
* Description: The server does not really do anything right now.  Just return.
*/
return_type fsServerUnmount(const int nparams, arg_type * a)
{
	ret.return_val = NULL;
	ret.return_size = 0;

	return ret;
}

/*
* Params: folderName
* Description: Returns pointer to stream for a file directory on server.
*/
return_type fsServerOpenDir(const int nparams, arg_type * a)
{
	// This procedure only takes 1 parameter, the folder name
	if( nparams != 1 ) {
		return fsServerErrorReturn();
	}

	// Construct fully qualified file path to search for
	char fullPath[ECE454_MAX_DIR_STR_SIZE];
	strcpy( fullPath, serverRootFolder );
	strcat( fullPath, (char*)(a->arg_val) );

	// Open the directory
	retDirPntr.val = (FSDIR*)opendir( fullPath );
	retDirPntr.error = errno;
	ret.return_val = (void*)&retDirPntr;
	ret.return_size = sizeof( retDirPntr );

	return( ret );
}

/*
* Params: pointer to FSDIR
* Description: Closes a directory for the calling client
*/
return_type fsServerCloseDir(const int nparams, arg_type * a)
{
	// This procedure only takes 1 parameter, the folder name
	if( nparams != 1 ) {
		return fsServerErrorReturn();
	}

	// Get the DIR pointer and call close on it
	DIR* myDir = *(DIR**)(a->arg_val);
	retDirIntVal.val = closedir( myDir );
	retDirIntVal.error = errno;

	ret.return_val = (void*)&retDirIntVal;
	ret.return_size = sizeof(retDirIntVal);

	return( ret );
}

/*
* Params: pointer to FSDIR
* Description: Reads the next entity in the specified directory
*/
return_type fsServerReadDir(const int nparams, arg_type * a)
{
	// This procedure only takes 1 parameter, the folder name
	if( nparams != 1 ) {
		return fsServerErrorReturn();
	}

	const int initErrno = errno;

	// Get the DIR pointer and read from the directory
	DIR* myDir = *(DIR**)(a->arg_val);
	struct dirent *d = readdir( myDir );

	retDent.dirPoint = (void*)d;

	if(d == NULL) {
		if(errno == initErrno) { errno = 0; }

		//return fsServerErrorReturn();
		retDent.error = errno;
	} else {

		// Populate the return structure
		if(d->d_type == DT_DIR) {
			retDent.val.entType = 1;
		}
		else if(d->d_type == DT_REG) {
			retDent.val.entType = 0;
		}
		else {
			retDent.val.entType = -1;
		}
		memcpy(&(retDent.val.entName), &(d->d_name), 256);
		retDent.error = errno;
	}

	ret.return_val = (void*)&retDent;
	ret.return_size = sizeof(retDent);

	return(ret);
}

/*
* Params: clientIP, clinetPID, fileName (relative to root), mode
* Description: 
*/
return_type fsServerOpen(const int nparams, arg_type * a)
{
	// This procedure only takes 4 parameters, the folder name
	if( nparams != 4 ) {
		return fsServerErrorReturn();
	}

	int flags = -1;
	int fileDesc = 0;
	fsLockOpCode_t myOp = OPEN;

	// Extract arguments
	arg_type *curr = a;
	char *clientIP = (char*)( curr->arg_val );

	curr = curr->next;
	pid_t clientPID = *(pid_t *)( curr->arg_val );

	curr = curr->next;
	char *fileName = (char*)( curr->arg_val );
	char fullPath[ECE454_MAX_DIR_STR_SIZE];			// Full file path
	strcpy( fullPath, serverRootFolder );
	strcat( fullPath, fileName );
#ifdef _DEBUG_1_
	printf("fsServerOpen(), 1 fullPath: %s, fileName: %s\n", fullPath, fileName); fflush(stdout);
#endif
	curr = curr->next;
	int mode = *(int*)( curr->arg_val );

	struct fsFileRecord myFileRecord;

	struct fsLockOpRequest myLockRequest;
	strcpy( myLockRequest.clientIP, clientIP );
	myLockRequest.clientPID = clientPID;
	myLockRequest.mode = myOp;


	// Set the open for READ for mode 0, and WRITE (with create) for mode 1.
	if(mode == ECE454_OPEN_MODE_READ)
	{
		flags = O_RDONLY;
	}
	else if(mode == ECE454_OPEN_MODE_WRITE)
	{
		flags = O_WRONLY | O_CREAT;
	}
	else
	{
		// Unknown mode value, return error.
		retDirIntVal.val = -1;
		retDirIntVal.error = EINVAL;
		ret.return_val = (void*)&retDirIntVal;
		ret.return_size = sizeof(retDirIntVal);
		return ret;
	}


	// Look for a file record for this file
	struct level1Node* fileNode = searchLevel1ByFileName(fileName);
	if( fileNode == NULL )
	{
#ifdef _DEBUG_1_
	printf("fsServerOpen(), 2 fullPath: %s, fileDesc: %d\n", fullPath, fileDesc); fflush(stdout);
#endif
		fileDesc = open(fullPath, flags, S_IRWXU);
		if (fileDesc != -1)
		{
			// Succeeded opening file
			populateFileRecord( &myFileRecord, fileName, fileDesc, 1, clientIP, clientPID, NULL );
			pushLevel1Node(myFileRecord);
		}
#ifdef _DEBUG_1_
	printf("fsServerOpen(), 3 fileName: %s, fileDesc: %d\n", fileName, fileDesc); fflush(stdout);
#endif

		retDirIntVal.val = fileDesc;
		retDirIntVal.error = errno;
		ret.return_val = (void*)&retDirIntVal;
		ret.return_size = sizeof(retDirIntVal);
	}
	else
	{
		// Check out the fild record
		myFileRecord = fileNode->data;

		// Check if the client is trying to re-open a file they already opened.  If so, error.
		if ((myFileRecord.hasLock != 0) && ( strcmp(myFileRecord.clientIP, clientIP)==0 ) && (myFileRecord.clientPID==clientPID))
		{
#ifdef _DEBUG_1_
	printf("fsServerOpen(), the client is trying to re-open a file they already opened, returning error\n"); fflush(stdout);
	printf("fsServerOpen(), myFileRecord.hasLock: %d\n", myFileRecord.hasLock); fflush(stdout);
	printf("fsServerOpen(), myFileRecord.clientIP: %s\n", myFileRecord.clientIP ); fflush(stdout);
	printf("fsServerOpen(), clientIP: %s\n", clientIP); fflush(stdout);
	printf("fsServerOpen(), myFileRecord.clientPID: %d\n", myFileRecord.clientPID); fflush(stdout);
	printf("fsServerOpen(), clientPID: %d\n",clientPID); fflush(stdout);
#endif
			retDirIntVal.val = -1;
			retDirIntVal.error = EADDRINUSE;
			ret.return_val = (void*)&retDirIntVal;
			ret.return_size = sizeof(retDirIntVal);
			return ret;
		}
		
		// Either append request to queue if not there
		if( !searchLevel2( myFileRecord.level2head, clientIP, clientPID, myOp) )
		{
			myFileRecord.level2head = pushLevel2Node( myFileRecord.level2head, myLockRequest);
		}

		// If the lock on the file is off and the current request is the next in the queue, let it have the lock
		struct fsLockOpRequest peekLockRequest = findNextRequestInLevel2(myFileRecord.level2head);

		if( (myFileRecord.hasLock==0) && ( strcmp(peekLockRequest.clientIP, clientIP)==0 ) && (peekLockRequest.clientPID==clientPID) && (peekLockRequest.mode==myOp) )
		{
			// Try to open the file
			fileDesc = open(fullPath, flags, S_IRWXU);

			// Remove request from queue after attempting operation
			populateFileRecord( &myFileRecord, fileName, fileDesc, (fileDesc != -1), clientIP, clientPID, myFileRecord.level2head );
			myFileRecord.level2head = popLevel2NodeList(myFileRecord.level2head);

			// Return the file descriptor
			retDirIntVal.val = fileDesc;
			retDirIntVal.error = errno;
		}
		else
		{
			// Return code that means retry.
			retDirIntVal.val = RPC_RETRY_CODE;
			retDirIntVal.error = errno;
		}

		// Check in and update the permanent file record
		fileNode->data = myFileRecord;

		// Return the file descriptor
		ret.return_val = (void*)&retDirIntVal;
		ret.return_size = sizeof(retDirIntVal);

	}
#ifdef _DEBUG_1_
	printf("fsServerOpen(), before return\n"); fflush(stdout);
#endif
	return(ret);
}

// Params: clientIP, clinetPID, file_Desciptor
//Returns 0 on success, -1 on failure
return_type fsServerClose(const int nparams, arg_type * a)
{
	// This procedure only takes 3 parameters, the folder name
	if( nparams != 3 ) {
		return fsServerErrorReturn();
	}

	fsLockOpCode_t myOp = CLOSE;


	// Extract arguments
	arg_type *curr = a;
	char *clientIP = (char*)( curr->arg_val );

	curr = curr->next;
	pid_t clientPID = *(pid_t *)( curr->arg_val );

	curr = curr->next;
	int fileDesc = *(int*)( curr->arg_val );

	struct fsFileRecord myFileRecord;

	struct fsLockOpRequest myLockRequest;
	strcpy( myLockRequest.clientIP, clientIP );
	myLockRequest.clientPID = clientPID;
	myLockRequest.mode = myOp;

#ifdef _DEBUG_1_
	printf("fsClose(), fileDesc: %d\n", fileDesc); fflush(stdout);
#endif

	// Look for a file record for this file
	struct level1Node* fileNode = searchLevel1ByFileDescriptor(fileDesc);
	if( fileNode == NULL )
	{
#ifdef _DEBUG_1_
	printf("fsClose(), fileNode is NULL\n"); fflush(stdout);
#endif
		// Since Level 1 queue is never reduced, this means the file was never opened.  This operation should fail.
		retDirIntVal.val = close(fileDesc);
		retDirIntVal.error = errno;
		// if (fileDesc != -1)
		// {
		// // Succeeded opening file
		// populateFileRecord( &myFileRecord, fileName, fileDesc, 1, clientIP, clientPID, NULL );
		// pushLevel1Node(myFileRecord);
		// }

		ret.return_val = (void*)&retDirIntVal;
		ret.return_size = sizeof(retDirIntVal);
	}
	else
	{
		// Check out the fild record
		myFileRecord = fileNode->data;

		// // Either append request to queue if not there
		// if( !searchLevel2( myFileRecord.level2head, clientIP, clientPID, myOp) )
		// {
		// myFileRecord.level2head = pushLevel2Node( myFileRecord.level2head, myLockRequest);
		// }

		// // If the lock on the file is off and the current request is the next in the queue, let it have the lock
		// struct fsLockOpRequest peekLockRequest = findNextRequestInLevel2(myFileRecord.level2head);

		// if( ( strcmp(peekLockRequest.clientIP, clientIP)==0 ) && (peekLockRequest.clientPID==clientPID) && (peekLockRequest.mode==myOp) )

		// Close file if opened by this client and is still currently open
		if( ( strcmp(myFileRecord.clientIP, clientIP)==0 ) && (myFileRecord.clientPID==clientPID) && (myFileRecord.hasLock != 0) )
		{
			// Try to close the file and save its return
			retDirIntVal.val = close(fileDesc);
			retDirIntVal.error = errno;

			// Remove request from queue after attempting operation
			populateFileRecord( &myFileRecord, "", fileDesc, 0, clientIP, clientPID, myFileRecord.level2head );
			//myFileRecord.level2head = popLevel2NodeList(myFileRecord.level2head);

		}
		else
		{
			// Return code that means retry.
			//retDirIntVal.val = RPC_RETRY_CODE;
			//retDirIntVal.error = errno;

			//Return a permissions error as client trying to close file they didn't open
			retDirIntVal.val = -1;
			retDirIntVal.error = EACCES;
		}

		// Check in and update the permanent file record
		fileNode->data = myFileRecord;

		// Return the file descriptor
		ret.return_val = (void*)&retDirIntVal;
		ret.return_size = sizeof(retDirIntVal);

	}

	return(ret);
}

/*
* Params: file_Desciptor, count
* Description: Read from the specified file
*/
return_type fsServerRead(const int nparams, arg_type * a)
{
	// This procedure only takes 2 parameter, the folder name
	if( nparams != 2 ) {
		return fsServerErrorReturn();
	}

	// Extract arguments
	int fileDesc = *(int*)(a->arg_val);
	unsigned int maxReadCount = *(unsigned int*)( a->next->arg_val );

	// Get read buffer ready.  Free old buffer and get new one.
	if ( retReadBuff != NULL ) { free( retReadBuff ); }
	retReadBuff = (char*)malloc( maxReadCount + 4 + 4 );		// Need 4 bytes at start for errno, and 4 bytes for readCount

	// Perform read
	int numBytesRead = (int)read(fileDesc, (void*)&retReadBuff[READ_BUF_DATA_IDX], (size_t) maxReadCount);
	*(int*)&retReadBuff[READ_BUF_NUMBYTES_IDX] = numBytesRead;
	*(int*)&retReadBuff[READ_BUF_ERRNO_IDX] = errno;			// Errno

	// Fill out return_type
	ret.return_val = (void*)retReadBuff;
	ret.return_size = numBytesRead < 0 ? 8 : (numBytesRead + 8);			// 4 bytes at start for error
	return ret;


}

/*
* Params: file_Descriptor, writeBuffer, count
* Description: Write to the specified file
*/
return_type fsServerWrite(const int nparams, arg_type * a)
{
	// This procedure only takes 1 parameter, the folder name
	if( nparams != 3 ) {
		return fsServerErrorReturn();
	}

	// Get args
	arg_type *curr = a;
	int temp_fd = *(int*)(curr->arg_val);

	curr = curr->next;
	void* temp_buf = (void*)(curr->arg_val);

	curr = curr->next;
	unsigned int temp_count = *(unsigned int *)(curr->arg_val);
#ifdef _DEBUG_1_
	printf("fsServerWrite(), temp_fd: %d\n",temp_fd); fflush(stdout);
	printf("fsServerWrite(), temp_buf: %s\n",temp_buf); fflush(stdout);
#endif
	// Perform write
	retDirIntVal.val = (int)write(temp_fd, temp_buf, (size_t)temp_count);
	retDirIntVal.error = errno;
	ret.return_val = (void*)&retDirIntVal;
	ret.return_size = sizeof(retDirIntVal);
#ifdef _DEBUG_1_
	perror("fsServerWrite()"); fflush(stdout);
#endif

	return(ret); 
}

// Params: clientIP, clinetPID, fileName
return_type fsServerRemove(const int nparams, arg_type * a)
{
	// This procedure only takes 3 parameters, the folder name
	if( nparams != 3 ) {
		return fsServerErrorReturn();
	}


	int fileDesc = 0;
	fsLockOpCode_t myOp = REMOVE;


	// Extract arguments
	arg_type *curr = a;
	char *clientIP = (char*)( curr->arg_val );

	curr = curr->next;
	pid_t clientPID = *(pid_t *)( curr->arg_val );

	curr = curr->next;
	char *fileName = (char*)( curr->arg_val );
	char fullPath[ECE454_MAX_DIR_STR_SIZE];			// Full file path
	strcpy( fullPath, serverRootFolder );
	strcat( fullPath, fileName );


	struct fsFileRecord myFileRecord;

	struct fsLockOpRequest myLockRequest;
	strcpy( myLockRequest.clientIP, clientIP );
	myLockRequest.clientPID = clientPID;
	myLockRequest.mode = myOp;


	// Look for a file record for this file
	struct level1Node* fileNode = searchLevel1ByFileName(fileName);
	if( fileNode == NULL )
	{
		retDirIntVal.val = remove(fullPath);
		retDirIntVal.error = errno;
		// if (fileDesc != -1)
		// {
		// // Succeeded opening file
		// populateFileRecord( &myFileRecord, fileName, fileDesc, 1, clientIP, clientPID, NULL );
		// pushLevel1Node(myFileRecord);
		// }

		ret.return_val = (void*)&retDirIntVal;
		ret.return_size = sizeof(retDirIntVal);
	}
	else
	{
		// Check out the fild record
		myFileRecord = fileNode->data;

		// Either append request to queue if not there
		if( !searchLevel2( myFileRecord.level2head, clientIP, clientPID, myOp) )
		{
			myFileRecord.level2head = pushLevel2Node( myFileRecord.level2head, myLockRequest);
		}

		// If the lock on the file is off and the current request is the next in the queue, let it have the lock
		struct fsLockOpRequest peekLockRequest = findNextRequestInLevel2(myFileRecord.level2head);

		if( (myFileRecord.hasLock==0) && ( strcmp(peekLockRequest.clientIP, clientIP)==0 ) && (peekLockRequest.clientPID==clientPID) && (peekLockRequest.mode==myOp) )
		{
			// Try to remove the file and save its return
			retDirIntVal.val = remove(fullPath);
			retDirIntVal.error = errno;

			// Remove request from queue after attempting operation
			populateFileRecord( &myFileRecord, fileName, fileDesc, 0, clientIP, clientPID, myFileRecord.level2head );
			myFileRecord.level2head = popLevel2NodeList(myFileRecord.level2head);

		}
		else
		{
			// Return code that means retry.
			retDirIntVal.val = RPC_RETRY_CODE;
			retDirIntVal.error = errno;
		}

		// Check in and update the permanent file record
		fileNode->data = myFileRecord;

		// Return the file descriptor
		ret.return_val = (void*)&retDirIntVal;
		ret.return_size = sizeof(retDirIntVal);

	}

	return(ret);
}
