#ifndef _ECE_FS_RPC_DATATYPES_
#define _ECE_FS_RPC_DATATYPES_

#include "ece454_fs.h"

#define READ_BUF_ERRNO_IDX 0
#define READ_BUF_NUMBYTES_IDX 4
#define READ_BUF_DATA_IDX 8

typedef enum {GOOD, BAD, RETRY} RPC_Ack_t;	// The types of acknowledgements that the RPC can give the client.

struct sRetAck {
	RPC_Ack_t val;
	int error;
};				// Acknowledgement from server for certain RPC calls

struct sRetDir {
	FSDIR* val;
	int error;
};				// Stores pointer to FSDIR so RPC can send the pointer itself.  Does not need to persist after that.

struct sRetInt {
	int val;
	int error;
};				// Stores an int value so RPC can send it.

struct sRetDent {
	struct fsDirent val;
	void* dirPoint;			// To check if null
	int error;
};					// File directory entity to return

#endif
