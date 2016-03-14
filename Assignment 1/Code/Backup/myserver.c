#include <stdio.h>
#include "ece454rpc_types.h"
int ret_int;
return_type r;
return_type add(const int nparams, arg_type* a)
{
	if(nparams != 2) {
		/* Error! */
		r.return_val = NULL;
		r.return_size = 0;
		return r;
	}
	if(a->arg_size != sizeof(int) || a->next->arg_size != sizeof(int))
	{
		/* Error! */
		r.return_val = NULL;
		r.return_size = 0;
		return r;
	}
	int i = *(int *)(a->arg_val);
	int j = *(int *)(a->next->arg_val);
	ret_int = i+j;
	r.return_val = (void *)(&ret_int);
	r.return_size = sizeof(int);
	return r;
}

return_type subtract(const int nparams, arg_type* a)
{
	if(nparams != 2) {
		/* Error! */
		r.return_val = NULL;
		r.return_size = 0;
		return r;
	}
	if(a->arg_size != sizeof(int) || a->next->arg_size != sizeof(int))
	{
		/* Error! */
		r.return_val = NULL;
		r.return_size = 0;
		return r;
	}
	int i = *(int *)(a->arg_val);
	int j = *(int *)(a->next->arg_val);
	ret_int = i-j;
	r.return_val = (void *)(&ret_int);
	r.return_size = sizeof(int);
	return r;
}

int main()
{
	register_procedure("addtwo", 2, add);
	register_procedure("subtracttwo", 2, subtract);
	register_procedure("subtractthree", 3, subtract);
	register_procedure("subtractfour", 4, subtract);
	register_procedure("addfour", 4, add);
	launch_server();
	/* should never get here, because
	launch_server(); runs forever. */
	return 0;
}
