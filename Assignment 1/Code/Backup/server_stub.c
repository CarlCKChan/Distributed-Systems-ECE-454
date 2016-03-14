/******************************************************************************************************
 * Names		:	Carl Chan, Debin Li
 * Course		:	ECE454
 * Assignment	:	1
 * Description	:	This holds code to run a simple RPC server.  It will exchange UDP packets with
 *					matching clients.
 ******************************************************************************************************/


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>					// To get arguments (since variable length)
#include "ece454rpc_types.h"		// Professor-defined types
#include "mybind.c"					// Custom 
#include <ifaddrs.h>				// For finding network addresses
#include <arpa/inet.h>				// For inet_htons()
#include "ece454_helper.h"			// Helper functions for this assignment



//#define UDP_MESSAGE_SIZE 1000		// The max size of the message 
//#define MAX_RPC_PROC_NAME 100		// Maximum character size for RPC procedure names
//#define MAX_RPC_PROC_REG 100		// The maximum number of functions that can be registered as RPC procedures


// This holds data about stored RPC procedures for this assignment.  This includes a procedure name, number 
// of parameters, and a pointer to the function.
struct ECE454_RPCProcedure {
	char procedure_name[MAX_RPC_PROC_NAME];
	int nparams;
	fp_type fnpointer;
};

// A list of RPC procedures stored.
struct ECE454_RPCProcedure ECE454_Procedures[MAX_RPC_PROC_REG];

// The current number of RPC procedures stored in 'ECE454_Procedures'
int ECE454_ProcedureListSize = 0;


/*
 * Determine if the passed parameters match a registered RPC procedure.  This means the procedure name
 * and number of parameters must be the same.
 * Returns: True on match, false otherwise.
 */
bool ECE454_match_procedure( const char *procName, const int nparams, struct ECE454_RPCProcedure proc )
{
	return( ( strcmp(procName, proc.procedure_name) == 0 ) && ( nparams == proc.nparams ) );
}


/*
 * Register a function as an RPC procedure.  If an existing procedure has a matching name and number
 * of parameters, it will simply replace the function pointer.
 * Returns: True on match, false otherwise.
 */
bool register_procedure(const char *procedure_name, const int nparams, fp_type fnpointer)
{
	bool alreadyThere = false;
	
	// Search through the existing registered procedures and update pointer if there is a match
	// Matches are done on both procedure name and number of parameters
	int i =0;
	for( i = 0; i < ECE454_ProcedureListSize; i++ )
	{
		if ( ECE454_match_procedure( procedure_name, nparams, ECE454_Procedures[i] ) )
		{
			alreadyThere = true;
			ECE454_Procedures[i].fnpointer = fnpointer;
			break;
		}
	}
	
	// If no existing matching procedure, create a new one
	if(!alreadyThere)
	{
		struct ECE454_RPCProcedure newProc;
		
		// Populate the procedure name, number of parameters, and function pointer
		strcpy( newProc.procedure_name, procedure_name );
		newProc.nparams = nparams;
		newProc.fnpointer = fnpointer;
		
		// Add new RPC procedure
		ECE454_Procedures[ ECE454_ProcedureListSize ] = newProc;
		ECE454_ProcedureListSize++;
	}
	
	return true;
}


/*
 * Used on the server side to call a registered function, based on a name and number of parameters.  The parameters
 * are passed as a pointer into the appropirate place in the receive buffer of the request from the client.
 * The structure of a client request is described in ece454_helper.h.
 *
 * Returns a return_type holding the desired return data.
 */
return_type ECE454_call_function_from_proc_name( const char* procName, const int nparams, const char* parameters )
{
	bool procFound = false;					// Whether the desired RPC procedure was found.
	fp_type funcPointer = NULL;				// Function pointer to call
	arg_type arguments[ nparams ];			// Used to store the arguments parsed from "parameters"
	arg_type currArg;
	return_type reponse;					// The value to return
	int currParamOffset = 0;
	
	// Search through the existing registered procedures and update pointer if there is a match
	// Matches are done on both procedure name and number of parameters
	int i = 0;
	for( i = 0; i < ECE454_ProcedureListSize; i++ )
	{
		if ( ECE454_match_procedure( procName, nparams, ECE454_Procedures[i] ) )
		{
			procFound = true;
			funcPointer = ECE454_Procedures[i].fnpointer;
			break;
		}
	}
	
	if(!procFound)
	{
		//Construct the error message if desired procedure is not found.
		char str[200];
		sprintf(str, "Could not find procedure with name=%s and nparams=%i\n", procName, nparams);
		
		//Returning the error
		return ECE454_generic_RPC_error(str);
	}
	else
	{
		// Found the desired procedure.  Loop through all parameters and make a linked list of them
		int count =0;
		for( count=0; count < nparams; count++ )
		{
			// Current argument size
			currArg.arg_size = *((int*)&parameters[currParamOffset]);
			currParamOffset += 4;
			
			// Current argument value (as pointer)
			currArg.arg_val = (void*)&parameters[currParamOffset];
			currParamOffset += currArg.arg_size;
			
			// Link the arguments together as linked list (last has NULL next pointer)
			if( count != (nparams-1) )
			{
				currArg.next = &arguments[count + 1];
			}
			else
			{
				currArg.next = NULL;
			}
			
			// Add the argument to the argument array
			arguments[count] = currArg;
		}
		
		// Call the function
		return (*funcPointer)(nparams, &arguments[0]);
	}
	
}


void launch_server()
{
	/*************************************** IMPORTANT VARIABES ***************************************/
	// This is a file descriptor for the UDP socket used for communication between server and client
	int udp_socket_fd;

	// This stores information needed by the socket on the local machine (address, port, and that it is IPv4).
	struct sockaddr_in udp_src_addr;
	
	// This stores information needed by the socket on the machine to send to (address, port, and that it is IPv4).
	struct sockaddr_in udp_dest_addr;
	int destAddrLen = sizeof(udp_dest_addr);
	
	// The message to be sent via UDP
	char message[UDP_MESSAGE_SIZE];
	
	// The buffer for data received from the server
	char recvBuffer[UDP_MESSAGE_SIZE];
	
	// The current offset in the message buffer where you want to start reading more data.  Starts at zero (start of buffer).
	int currMsgOffset = 0;
	// The current offset to start writing to the response buffer
	int currResponseOffset = 0;
	
	// The name and number of parameters for the current procedure being called
	char procName[MAX_RPC_PROC_NAME];
	int nparams = 0;
	
	// The current return to send back to client
	return_type currReturn;

	/****************************************** OPEN SOCKET ******************************************
	 * Try to open a UDP socket (IPv4).  Exit if there is an error (socket file descriptor is -1).
	 *************************************************************************************************/
	udp_socket_fd = socket(PF_INET, SOCK_DGRAM, 0);
	if( udp_socket_fd == -1 )
	{
		ECE454_generic_RPC_error("Could not create UDP socket.\n");
		return;
	}

	/****************************************** BIND SOCKET ******************************************
	 * Reaching this point means a UDP socket was properly opened.  Bind it to a port.
	 ************************************************************************************************/

	// Clear the socket address struct before using it for safety.
	memset((char *)&udp_src_addr, 0, sizeof(udp_src_addr));

	// Set the socket address with ths following settings in this order: Use IPv4, use a port number
	// assigned by the system, and use any address of the socket (do not care if there are multiple
	// network interfaces).
	udp_src_addr.sin_family = PF_INET;
	udp_src_addr.sin_port = htons(0); 
	udp_src_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		

	// Find the opened socket to the specified port.  If the return is -1 then an error occurred, in
	// which case exit
	//int bind_result = bind( udp_socket_fd, (struct sockaddr *)&udp_src_addr, sizeof(udp_src_addr) );
	int bind_result = mybind( udp_socket_fd, &udp_src_addr);
	if( bind_result == -1 )
	{
		ECE454_client_side_error("Could not bind UDP socket to specified port.\n", udp_socket_fd);
		return;
	}
	
	/****************************************** PRINT SOCKET INFO ******************************************
	 * Print out the IP and port of the server
	 ************************************************************************************************/
	//Find the port number the server socket is bound to
	struct sockaddr_in address_in;
	socklen_t clientSocketSize = sizeof(address_in);
	if(getsockname(udp_socket_fd ,(struct sockaddr *)&address_in, &clientSocketSize) < 0)
	{
		ECE454_client_side_error("Unable to find port of socket\n", udp_socket_fd);
		return;
	}
	
	//Get a list of IP addresses of the server machine's network interfaces
	struct ifaddrs *server_interface, *tempPointer = NULL;
	getifaddrs(&server_interface);
	tempPointer=server_interface;
	
	// Print out all the IPv4 addresses of the first Ethernet interface found, along with the server port
	for(tempPointer = server_interface; tempPointer!=NULL; tempPointer=tempPointer->ifa_next)
	{
		if ( (tempPointer->ifa_addr->sa_family==PF_INET) && (strcmp((*tempPointer).ifa_name,"eth0") == 0) )
		{
			char * addr=inet_ntoa(((struct sockaddr_in *)(tempPointer->ifa_addr))->sin_addr);
			printf("%s ",addr);
			printf( "%d\n", ntohs(address_in.sin_port));
			break;
		}
	}
	
	// Free the memory associated with the interface information
	freeifaddrs(server_interface);
	
	
	/**************************************** READ MESSAGES ****************************************
	 * Wait for a message and process it
	 ***********************************************************************************************/
	while( true )
	{
		// Reset offsets into the buffer
		currMsgOffset = 0;
		currResponseOffset = 0;
		
		// On successful receive, process the client request, calculate the return, and send a response.  On error, return error.
		if( recvfrom(udp_socket_fd, recvBuffer, UDP_MESSAGE_SIZE, 0, (struct sockaddr *)&udp_dest_addr, &destAddrLen) <= 0 )
		{
			currReturn = ECE454_generic_RPC_error("Could not receive message from client.\n");
		}
		else
		{
			// Successfully got message from client.  Get the name and number of parameters of the procedure.
			// The structure of a client request is described in ece454_helper.h.
			strcpy( procName, recvBuffer );
			currMsgOffset = strlen(procName) + 1;
			nparams = *((int*)&recvBuffer[ currMsgOffset ]);
			currMsgOffset += 4;
			
			// Find/call the desired function.
			currReturn = ECE454_call_function_from_proc_name( procName, nparams, &recvBuffer[currMsgOffset] );
		}
		
		// Construct response.  Packet format described in ece454_helper.h.
		currResponseOffset = ECE454_put_data_into_buffer_as_bytes( message, currResponseOffset, sizeof(int), (void*)&(currReturn.return_size) );
		currResponseOffset = ECE454_put_data_into_buffer_as_bytes( message, currResponseOffset, currReturn.return_size, currReturn.return_val );
		
		// Send response
		if (sendto(udp_socket_fd, message, sizeof(int) + currReturn.return_size, 0, (struct sockaddr *)&udp_dest_addr, destAddrLen) < 0)
		{
			printf("Could not send response to client\n");
		}
	}
}


/*
 * Prints out the passed error message and returns an error return_type (size 0, NULL pointer).
 *
 * The program should return using whatever is returned by this function.
 */
return_type ECE454_generic_RPC_error(const char *errorMessage)
{
	printf("%s", errorMessage);
	
	return_type errorReponse;
	errorReponse.return_val = NULL;
	errorReponse.return_size = 0;
	return errorReponse;
}


/* This is used if there is an issue on the client side.  Prints out an error, does miscellaneous cleanup (ex. closes UDP
 * socket), and returns a default error return structure.  It builds on the generic RPC error function.
 *
 * The program should return using whatever is returned by this function.
 */
return_type ECE454_client_side_error( const char *errorMessage, int UDPSocket )
{
	close(UDPSocket);
	return ECE454_generic_RPC_error(errorMessage);
}

