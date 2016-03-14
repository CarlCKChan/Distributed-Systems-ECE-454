#include <stdio.h>
#include "ece454rpc_types.h"
int main(int argc, char** argv)
{
	int a = -10, b = 20, c = 100, d = 57;;
	//return_type ans = make_remote_call("ecelinux3.uwaterloo.ca",10000,"addtwo", 2,sizeof(int), (void *)(&a), sizeof(int), (void *)(&b));
	return_type ans = make_remote_call(argv[1], atoi(argv[2]),"addtwo", 2,sizeof(int), (void *)(&a), sizeof(int), (void *)(&b));
	//int i = *(int *)(ans.return_val);
	testResult(ans, 10);
	
	ans = make_remote_call(argv[1], atoi(argv[2]),"subtracttwo", 2,sizeof(int), (void *)(&a), sizeof(int), (void *)(&b));
	//i = *(int *)(ans.return_val);
	testResult(ans, -30);
	
	
	ans = make_remote_call(argv[1], atoi(argv[2]),"addtwo", 2,sizeof(int), (void *)(&c), sizeof(int), (void *)(&d));
	//i = *(int *)(ans.return_val);
	testResult(ans, 157);
	
	ans = make_remote_call(argv[1], atoi(argv[2]),"subtractthree", 3,sizeof(int), (void *)(&a), sizeof(int), (void *)(&b), sizeof(int), (void *)(&c));
	//i = *(int *)(ans.return_val);
	testResult(ans, 0);
	
	ans = make_remote_call(argv[1], atoi(argv[2]),"subtractthree", 4,sizeof(int), (void *)(&a), sizeof(int), (void *)(&b), sizeof(int), (void *)(&c), sizeof(int), (void *)(&d));
	//i = *(int *)(ans.return_val);
	testResult(ans, 0);
	
	ans = make_remote_call(argv[1], atoi(argv[2]),"addfour", 2,sizeof(int), (void *)(&c), sizeof(int), (void *)(&d));
	//i = *(int *)(ans.return_val);
	testResult(ans, 0);
	
	ans = make_remote_call(argv[1], atoi(argv[2]),"sdafasdfsdafasdf", 2,sizeof(int), (void *)(&c), sizeof(int), (void *)(&d));
	//i = *(int *)(ans.return_val);
	testResult(ans, 0);
	
	
	return 0;
}

void testResult(return_type returnedResult, int expectedResult)
{
	if(returnedResult.return_size!=0)
	{
		printf("Expected: %d, Actual: %d\n", expectedResult, *((int *)returnedResult.return_val));
	}
	else
	{
		if(returnedResult.return_val==NULL)
		{
			printf("Returned error\n");
		}
	}
}
