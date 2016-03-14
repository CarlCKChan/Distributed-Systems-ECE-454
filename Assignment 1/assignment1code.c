#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>					// To get arguments (since variable length)
#include "ece454rpc_types.h"		// Professor-defined types

#DEFINE UDP_MESSAGE_SIZE 1000		// The max size of the message 
#DEFINE MAX_RPC_PROC_NAME 100		// Maximum character size for RPC procedure names
#DEFINE MAX_RPC_PROC_REG 100		// The maximum number of functions that can be registered as RPC procedures


/*
					UDP Message Contents : To server
		------------------------------------------------------- ...
		|		a		|	b	|	c	|		d		|		... more parameters
		------------------------------------------------------- ...
		
		a	: Procedure Name (null-terminated string)
		b	: Number of parameters (Int: 4 bytes)
		c+d	: Argument pair.
				c	: Size of this parameter (Int: 4 bytes)
				d	: Value of parameter (Int: 4 bytes)
				
				
					UDP Message Contents : To client
		-------------------------
		|	e	|		f		|
		-------------------------
		
		e	: Size of return value (Int: 4 bytes)
		f	: Return value (Variable size)
 */
 
 
// This holds data about stored RPC procedures for this assignment
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
 * Description: Adds data of a certain size to a buffer at the specified offset.
 * Return: The new offset to start putting more data into buffer at.
 */
int ECE454_put_data_into_buffer_as_bytes( const char *buffer, const int offset, const int size, const void* value )
{
	for( int i = 0; i < size; i++ )
	{
		buffer[offset + i] = ((char*)value)[i];
	}
	return( offset + size );
}


/* This is used if there is an issue with the RPC.  Prints out an error and returns a default error return structure.
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


// TODO : Get destination address to send to server.
return_type make_remote_call(const char *servernameorip, const int serverportnumber, const char *procedure_name, const int nparams, ...)
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
	
	// The current offset in the message buffer where you want to start adding more data.  Starts at zero (start of buffer).
	int currMsgOffset = 0;
	
	// To handle the variable number of parameters
	va_list arg_list;
	
	// The size and void pointer for the current variable argument being looked at
	int currArgSize = 0;
	void* currArgValue = 0;
	
	// The return_type returned by the server
	return_type serverReponse;


	/****************************************** OPEN SOCKET ******************************************
	 * Try to open a UDP socket (IPv4).  Exit if there is an error (socket file descriptor is -1).
	 *************************************************************************************************/
	udp_socket_fd = socket(PF_INET, SOCK_DGRAM, 0);
	if( udp_socket_fd == -1 )
	{
		return ECE454_generic_RPC_error("Could not create UDP socket.");
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
	int bind_result = bind( udp_socket_fd, (struct sockaddr *)&udp_src_addr, sizeof(udp_src_addr) );
	if( bind_result == -1 )
	{
		return ECE454_client_side_error("Could not bind UDP socket to specified port.", udp_socket_fd);
	}
	
	/************************************** CONSTRUCT MESSAGE **************************************
	 * Populate the message buffer to send via UDP.
	 ***********************************************************************************************/
	// Copy the procedure name into the message and advance the message offset pointer.
	// The offset pointer is the length of the procedure name plus one character (for null terminator)
	strcpy(message, procedure_name);
	currMsgOffset += (strlen( procedure_name ) + 1);
	
	// Put the number of parameters into the messageand update message offset
	currMsgOffset = ECE454_put_data_into_buffer_as_bytes( message, currMsgOffset, sizeof(int), (void*)&nparams );
	//message[currMsgOffset] = (nparams >> 24) & 0xFF;
	//message[currMsgOffset+1] = (nparams >> 16) & 0xFF;
	//message[currMsgOffset+2] = (nparams >> 8) & 0xFF;
	//message[currMsgOffset+3] = nparams & 0xFF;
	
	
	// Put the arguments into the message
	// Initialize the variable argument list and loop over them
	va_start( arg_list, nparams );
	for( int i = 0; i < nparams; i++ )
	{
		// Put size of current argument into the message buffer
		currArgSize = va_arg( a_list, int );
		currMsgOffset = ECE454_put_data_into_buffer_as_bytes( message, currMsgOffset, sizeof(int), (void*)&currArgSize );
		
		// Put the value of the current argument into the message buffer
		currArgValue = va_arg( a_list, void* );
		currMsgOffset = ECE454_put_data_into_buffer_as_bytes( message, currMsgOffset, currArgSize, currArgValue );
	}
	va_end( arg_list );
	
	
	/************************************** SEND MESSAGE **************************************
	 * Send the UDP message to the server and get the response.
	 ***********************************************************************************************/
	if( sendto(udp_socket_fd, (void*)&message[0], currMsgOffset, 0, (struct sockaddr *)&udp_dest_addr, destAddrLen) != -1)
	{
		// If sent successfully, get the response from server
		if( recvfrom(udp_socket_fd, (void*)&revcBuffer[0], UDP_MESSAGE_SIZE, 0, (struct sockaddr *)&udp_dest_addr, &destAddrLen) != -1 )
		{
			// Extract the data.  Read size of return and use it to allocate heap memory to store the return value.
			// The return value can be found after the size in the receive buffer (displacement of sizeof(int) bytes).
			serverReponse.return_size = *((int*)&revcBuffer[0]);
			void* return_val = malloc( serverReponse.return_size );
			ECE454_put_data_into_buffer_as_bytes( (char*)return_val, 0, serverReponse.return_size, (void*)&revcBuffer[ sizeof(int) ] );
			serverReponse.return_val = return_val;
			
			return serverReponse;
		}
		else
		{
			return ECE454_client_side_error("Could not get response from server.", udp_socket_fd);
		}
	}
	else
	{
		return ECE454_client_side_error("Could not send UDP packet.", udp_socket_fd);
	}
	
	// Close the UDP socket
	close(udp_socket_fd);
	
}


/* 
 * Returns whether or not the name and number of parameters of an RPCProcedure match the passed values.
 */
bool ECE454_match_procedure( const char *procName, const int nparams, struct ECE454_RPCProcedure proc )
{
	return( ( strcmp(procName, proc.procedure_name) == 0 ) && ( nparams == proc.nparams ) );
}


bool register_procedure(const char *procedure_name, const int nparams, fp_type fnpointer)
{
	bool alreadyThere = false;
	
	// Search through the existing registered procedures and update pointer if there is a match
	// Matches are done on both procedure name and number of parameters
	for( int i = 0; i < ECE454_ProcedureListSize; i++ )
	{
		if ( ECE454_match_procedure( procedure_name, nparams, ECE454_Procedures[i] ) )
		{
			alreadyThere = true;
			ECE454_Procedures[i].fnpointer = fnpointer;
			break;
		}
	}
	
	// If no existing matching procedure, create a new one
	if( alreadyThere == false )
	{
		struct ECE454_RPCProcedure newProc;
		strcpy( newProc.procedure_name, procedure_name );
		newProc.nparams = nparams;
		newProc.fnpointer = fnpointer;
		
		ECE454_Procedures[ ECE454_ProcedureListSize ] = newProc;
		ECE454_ProcedureListSize++;
	}
}


/*
 * Used on the server side to call a registered function, based on a name and number of parameters.
 * Returns a return_type holding the desired return data.
 */
return_type ECE454_call_function_from_proc_name( const char* procName, const int nparams, const char* parameters )
{
	bool procFound = false;
	fp_type funcPointer = NULL;
	arg_type arguments[ nparams ];
	arg_type currArg;
	return_type reponse;
	int currParamOffset = 0;
	
	// Search through the existing registered procedures and update pointer if there is a match
	// Matches are done on both procedure name and number of parameters
	for( int i = 0; i < ECE454_ProcedureListSize; i++ )
	{
		if ( ECE454_match_procedure( procName, nparams, ECE454_Procedures[i] ) )
		{
			procFound = true;
			funcPointer = ECE454_Procedures[i].fnpointer;
			break;
		}
	}
	
	if( procFound == false )
	{
		return ECE454_generic_RPC_error("Could not find procedure with name=%s and nparams=%i", procName, nparams);
	}
	else
	{
		for( int count=0; count < nparams; count++ )
		{
			currArg.arg_size = *((int*)&parameters[currParamOffset]);
			currParamOffset += 4;
			
			currArg.arg_val = (void*)&parameters[currParamOffset];
			currParamOffset += currArg.arg_size;
			
			if( count != (nparams-1) )
			{
				currArg.next = &arguments[count + 1];
			}
			else
			{
				currArg.next = NULL;
			}
			
			arguments[count] = currArg;
		}
		
		return funcPointer(nparams, &arguments[0]);
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
	
	// The current return to send back ot client
	return_type currReturn;


	/****************************************** OPEN SOCKET ******************************************
	 * Try to open a UDP socket (IPv4).  Exit if there is an error (socket file descriptor is -1).
	 *************************************************************************************************/
	udp_socket_fd = socket(PF_INET, SOCK_DGRAM, 0);
	if( udp_socket_fd == -1 )
	{
		ECE454_generic_RPC_error("Could not create UDP socket.");
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
	int bind_result = bind( udp_socket_fd, (struct sockaddr *)&udp_src_addr, sizeof(udp_src_addr) );
	if( bind_result == -1 )
	{
		ECE454_client_side_error("Could not bind UDP socket to specified port.", udp_socket_fd);
		return;
	}
	
	
	/**************************************** READ MESSAGES ****************************************
	 * Wait for a message and process it
	 ***********************************************************************************************/
	while( true )
	{
		if( recvfrom(udp_socket_fd, recvBuffer, UDP_MESSAGE_SIZE, 0, (struct sockaddr *)&udp_dest_addr, &destAddrLen) <= 0 )
		{
			currReturn = ECE454_generic_RPC_error("Could not receive message from client.");
		}
		else
		{
			// Successfully got message from client.
			// Get the name and number of parameters of the procedure
			strcpy( procName, recvBuffer );
			currMsgOffset = strlen(procName) + 1;
			nparams = *((int*)&recvBuffer[ currMsgOffset ]);
			currMsgOffset += 4;
			
			currReturn = ECE454_call_function_from_proc_name( procName, nparams, &recvBuffer[currMsgOffset] );
		}
		
		// Construct response
		currResponseOffset = ECE454_put_data_into_buffer_as_bytes( message, currResponseOffset, sizeof(int), (void*)&(currReturn.return_size) );
		currResponseOffset = ECE454_put_data_into_buffer_as_bytes( message, currResponseOffset, currReturn.return_size, currReturn.return_val );
		
		// Send response
		if (sendto(udp_socket_fd, message, sizeof(int) + currReturn.return_size, 0, (struct sockaddr *)&udp_dest_addr, destAddrLen) < 0)
		{
			printf("Could not send response to client");
		}
	}
	
}