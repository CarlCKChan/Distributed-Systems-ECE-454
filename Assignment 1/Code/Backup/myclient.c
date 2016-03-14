#include <stdio.h>
#include "ece454rpc_types.h"
int main(int argc, char** argv)
{
	int a = -10, b = 20;
	//return_type ans = make_remote_call("ecelinux3.uwaterloo.ca",10000,"addtwo", 2,sizeof(int), (void *)(&a), sizeof(int), (void *)(&b));
	/*return_type ans = make_remote_call(argv[1], atoi(argv[2]),"addtwo", 2,sizeof(int), (void *)(&a), sizeof(int), (void *)(&b));
	int i = *(int *)(ans.return_val);
	printf("client, got result: %d\n", i);
	ans = make_remote_call(argv[1], atoi(argv[2]),"subtracttwo", 2,sizeof(int), (void *)(&a), sizeof(int), (void *)(&b));
	i = *(int *)(ans.return_val);
	printf("client, got result: %d\n", i);
	
	
	ans = make_remote_call(argv[1], atoi(argv[2]),"addtwo", 2,sizeof(int), (void *)(&c), sizeof(int), (void *)(&d));
	i = *(int *)(ans.return_val);
	printf("client, got result: %d\n", i);*/
	int c = 100, d = 57;
	return_type ans = make_remote_call(argv[1], atoi(argv[2]),"subtractthree", 3,sizeof(int), (void *)(&a), sizeof(int), (void *)(&b), sizeof(int), (void *)(&c));
	int i = *(int *)(ans.return_val);
	printf("client, got result: %d\n", i);
	
	/*ans = make_remote_call(argv[1], atoi(argv[2]),"subtractthree", 4,sizeof(int), (void *)(&a), sizeof(int), (void *)(&b), sizeof(int), (void *)(&c), sizeof(int), (void *)(&d));
	i = *(int *)(ans.return_val);
	printf("client, got result: %d\n", i);*/
	
	ans = make_remote_call(argv[1], atoi(argv[2]),"addfour", 2,sizeof(int), (void *)(&c), sizeof(int), (void *)(&d));
	i = *(int *)(ans.return_val);
	printf("client, got result: %d\n", i);
	
	
	return 0;
}
