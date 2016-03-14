/* 
                    UDP Message Contents : To server 
        ------------------------------------------------------- ... 
        |       a       |   b   |   c   |       d       |       ... more parameters 
        ------------------------------------------------------- ... 
          
        a   : Procedure Name (null-terminated string) 
        b   : Number of parameters (Int: 4 bytes) 
        c+d : Argument pair. 
                c   : Size of this parameter (Int: 4 bytes) 
                d   : Value of parameter (Int: 4 bytes) 
                  
                  
                    UDP Message Contents : To client 
        ------------------------- 
        |   e   |       f       | 
        ------------------------- 
          
        e   : Size of return value (Int: 4 bytes) 
        f   : Return value (Variable size) 
 */


#define UDP_MESSAGE_SIZE 1000		// The max size of the message 
#define MAX_RPC_PROC_NAME 100		// Maximum character size for RPC procedure names
#define MAX_RPC_PROC_REG 100		// The maximum number of functions that can be registered as RPC procedures

/* This is used if there is an issue with the RPC.  Prints out an error and returns a default error return structure.
 *
 * The program should return using whatever is returned by this function.
 */
extern return_type ECE454_generic_RPC_error(const char *errorMessage);


/* This is used if there is an issue on the client side.  Prints out an error, does miscellaneous cleanup (ex. closes UDP
 * socket), and returns a default error return structure.  It builds on the generic RPC error function.
 *
 * The program should return using whatever is returned by this function.
 */
extern return_type ECE454_client_side_error( const char *errorMessage, int UDPSocket );
