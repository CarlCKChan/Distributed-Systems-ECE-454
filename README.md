# Distributed-Systems-ECE-454
Collection of work done for ECE454 (Distributed Systems).  This is both code (RPC and distributed file system) and theoretical assignments.

## Programming
### Assignment 1
A simple RPC (Remote Procedure Call) system.  The server registers functions under a certain name.  The client can then make the server run the functions and return the answer.  The RPC systems facilitates the packing/unpacking and communication between the client and server.

### Assignment 4
A distributed file system built on top of the previous RPC system.  Clients aquire a lock to read and write to a file located on the server on a first-come-first-serve basis.  Clients must wait until all others in front of the queue release the lock.

## Theoretical
All other assignments are calculations and proofs involving different aspects of distributed systems.
