#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>					// To get arguments (since variable length)
#include "ece454rpc_types.h"		// Professor-defined types
#include "ece454_helper.h"			// Helper functions

/*
 * Description: Adds data of a certain size to a buffer at the specified offset.
 * Return: The new offset to start putting more data into buffer at.
 */
int ECE454_put_data_into_buffer_as_bytes(char *buffer, const int offset, const int size, const void* value )
{
	int i = 0;
	for( i = 0; i < size; i++ )
	{
		buffer[offset + i] = ((char*)value)[i];
	}
	//Returning the new "offset"
	return( offset + size );
}

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
	
	// Clear the socket address struct before using it for safety.
	memset((char *)&udp_dest_addr, 0, destAddrLen);

	struct addrinfo destAddrInfo;
	
	//Making a "hint" of PF_INET family, and for UDP Datagrams
	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = PF_INET;
	hints.ai_socktype=SOCK_DGRAM;
	
	struct addrinfo *destAddrSearchResult;
	memset(&destAddrInfo, 0, sizeof(destAddrInfo));
	
	// Set the socket address with this following settings in this order: Use IPv4, use a port number
	// passed by caller, and lookup the address from the passed domain name/IP (use the first address given).
	udp_dest_addr.sin_family = PF_INET;
	udp_dest_addr.sin_port = htons(serverportnumber);
	//Searching for the IP address, then put the address information into sin_addr
	getaddrinfo(servernameorip, NULL, (const struct addrinfo *)&hints, &destAddrSearchResult);
	udp_dest_addr.sin_addr = ((struct sockaddr_in *)(destAddrSearchResult->ai_addr))->sin_addr;
	freeaddrinfo(destAddrSearchResult);
	
	/************************************** CONSTRUCT MESSAGE **************************************
	 * Populate the message buffer to send via UDP.
	 ***********************************************************************************************/
	// Copy the procedure name into the message and advance the message offset pointer.
	// The offset pointer is the length of the procedure name plus one character (for null terminator)
	strcpy(message, procedure_name);
	currMsgOffset += (strlen( procedure_name ) + 1);
	
	// Put the number of parameters into the messageand update message offset
	currMsgOffset = ECE454_put_data_into_buffer_as_bytes( message, currMsgOffset, sizeof(int), (void*)&nparams );
	
	
	// Put the arguments into the message
	// Initialize the variable argument list and loop over them
	va_start( arg_list, nparams );
	int i = 0;
	for( i = 0; i < nparams; i++ )
	{
		// Put size of current argument into the message buffer
		currArgSize = va_arg( arg_list, int );
		currMsgOffset = ECE454_put_data_into_buffer_as_bytes( message, currMsgOffset, sizeof(int), (void*)&currArgSize );
		
		// Put the value of the current argument into the message buffer
		currArgValue = va_arg( arg_list, void* );
		currMsgOffset = ECE454_put_data_into_buffer_as_bytes( message, currMsgOffset, currArgSize, currArgValue );
	}
	va_end( arg_list );
	
	
	/************************************** SEND MESSAGE **************************************
	 * Send the UDP message to the server and get the response.
	 ***********************************************************************************************/
	if( sendto(udp_socket_fd, (void*)&message[0], currMsgOffset, 0, (struct sockaddr *)&udp_dest_addr, destAddrLen) != -1)
	{
		// If sent successfully, get the response from server
		if( recvfrom(udp_socket_fd, (void*)&recvBuffer[0], UDP_MESSAGE_SIZE, 0, (struct sockaddr *)&udp_dest_addr, &destAddrLen) != -1 )
		{
			// Extract the data.  Read size of return and use it to allocate heap memory to store the return value.
			// The return value can be found after the size in the receive buffer (displacement of sizeof(int) bytes).
			serverReponse.return_size = *((int*)&recvBuffer[0]);
			void* return_val = malloc( (size_t)serverReponse.return_size );
			ECE454_put_data_into_buffer_as_bytes( (char*)return_val, 0, serverReponse.return_size, (void*)&recvBuffer[ sizeof(int) ] );
			serverReponse.return_val = return_val;
			
			if(serverReponse.return_size==0)
			{
				serverReponse.return_val = NULL;
			}
			else
			{
				void* return_val = malloc( (size_t)serverReponse.return_size );
				ECE454_put_data_into_buffer_as_bytes( (char*)return_val, 0, serverReponse.return_size, (void*)&recvBuffer[ sizeof(int) ] );
				serverReponse.return_val = return_val;
			}
			
			return serverReponse;

		}
		else
		{
			return ECE454_client_side_error("Could not get response from server.\n", udp_socket_fd);
		}
	}
	else
	{
		return ECE454_client_side_error("Could not send UDP packet.\n", udp_socket_fd);
	}
	
	// Close the UDP socket
	close(udp_socket_fd);	
}
