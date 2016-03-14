#include "ece454_fs.h"
#include <string.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdlib.h>

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

struct level1Node* searchByFileName(char* stringToSearchFor)
{
	struct level1Node* current = level1head;
	if(current==NULL)
	{
		return NULL;
	}
	while(current->next!=NULL)
	{
		if(strcmp(current->data.fileName,stringToSearchFor)==0)
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

//Need to append to the front of the list
void pushLevel2Node(struct level2Node* level2head,struct fsLockOpRequest input)
{
    struct level2Node *temp;
    temp= (struct level2Node *)malloc(sizeof(struct level2Node));
    temp->data=input;
	temp->next=level2head;
	level2head = temp;
}

//Delete from the back of the list
struct level2Node* popLevel2NodeList(struct level2Node* level2Head)
{
	struct level2Node *previous, *current;
	struct level2Node* popLevel2Node;
	popLevel2Node=NULL;
	current = level2Head;
	previous=current;
	if(current==NULL)
	{
		return popLevel2Node;
	}
	while(current->next!=NULL)
	{
		previous=current;
		current=current->next;
	}
	if(previous==current)
	{
		popLevel2Node=previous;
		//free(previous);
		level2Head=NULL;
		return popLevel2Node;
	}
	previous->next=NULL;
	//free(current);
	popLevel2Node=current;
	return popLevel2Node;
}
//End Level 2 Linked List

//Testing
void addToLevel1Queue(char* fileName, int fileDescriptor, bool hasLock, char* clientIP)
{
	struct fsFileRecord temp;
	strcpy(temp.fileName, fileName);
	temp.fileDesciptor=fileDescriptor;
	temp.hasLock=hasLock;
	strcpy(temp.clientIP, clientIP);
	pushLevel1Node(temp);
}
void queueTester()
{
	addToLevel1Queue("Test1", 0001, true, "ClientIP1");
	addToLevel1Queue("Test2", 0002, true, "ClientIP2");
	addToLevel1Queue("Test3", 0003, true, "ClientIP3");
	addToLevel1Queue("Test4", 0004, true, "ClientIP4");
	addToLevel1Queue("Test5", 0005, true, "ClientIP5");
}

struct level1Node* queueSearcher()
{
	struct level1Node* result = searchByFileName("Test3");
	if(result!=NULL)
	{
		printf("found: %s \n" , result->data.fileName);
	}
	return result;
}

void testDelete()
{
	removeLevel1Node("Test3");
	removeLevel1Node("Test1");
	removeLevel1Node("Test5");
	removeLevel1Node("Test2");
	removeLevel1Node("Test4");
}

int main()
{
	queueTester();
	struct level1Node* current=level1head;
	while(current!=NULL)
	{
		printf("%s\n",current->data.fileName);
		current=current->next;
	}
	printf("Delete:\n");
	testDelete();
	current=level1head;
	while(current!=NULL)
	{
		printf("yolo");
		printf("%s\n",current->data.fileName);
		current=current->next;
	}
}
