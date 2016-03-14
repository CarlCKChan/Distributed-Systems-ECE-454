/* Mahesh V. Tripunitara
   University of Waterloo
   Part of a simplified RPC implementation
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ece454rpc_types.h"

#if 0
#define _DEBUG_1_
#endif

/* We allocate a global variable for returns. However, 
 * r.return_val is allocated dynamically. We look to
 * the server_stub to deallocate after it returns the
 * response to the client. */

return_type r;

extern printRegisteredProcedures();

return_type nothing(const int nparams, arg_type *a) {
    /* Takes no args, and returns nothing meaningful */
#if 0
    printf("nothing() invoked at server\n"); fflush(stdout);
#endif
    r.return_size = 0;
    r.return_val = NULL;

#if 0
    printf("nothing() returning at server\n"); fflush(stdout);
#endif

    return r;
}

return_type add(const int nparams, arg_type* a)
{
    if(nparams != 2) {
	/* Error! */
	r.return_val = NULL;
	r.return_size = 0;
	return r;
    }

    if(a->arg_size != sizeof(int) ||
       a->next->arg_size != sizeof(int)) {
	/* Error! */
	r.return_val = NULL;
	r.return_size = 0;
	return r;
    }

    int i = *(int *)(a->arg_val);
    int j = *(int *)(a->next->arg_val);

    int *ret_int = (int *)malloc(sizeof(int));

    *ret_int = i+j;
    r.return_val = (void *)(ret_int);
    r.return_size = sizeof(int);

    return r;
}

return_type pickFirst(const int nparams, arg_type *a) {
    /* Pick the first of a list of stuff & return it */
    if(nparams <= 0) {
	r.return_val = NULL;
	r.return_size = 0;
    }
    else {
	r.return_val = (void *)malloc(a->arg_size);
	memcpy(r.return_val, a->arg_val, a->arg_size);
	r.return_size = a->arg_size;
    }

    return r;
}

/* compar() below for use by qsort() */
int compar(const void *a, const void *b) {
    int i = *(int *)a;
    int j = *(int *)b;

    return i - j;
}

return_type max(const int nparams, arg_type *a) {
    /* Interprets the one arg as an array of integers.
     * Returns them sorted.
     * Note: we sort the arg a "in place". So no
     * new memory is allocated. */

    int *ia = (int *)malloc(a->arg_size);
    memcpy(ia, a->arg_val, a->arg_size);
    int n = (a->arg_size)/(sizeof(int));

    if(n < 0) {
	r.return_val = NULL;
	r.return_size = 0;
    }
    else {
	qsort(ia, n, sizeof(int), compar);
	r.return_size = (a->arg_size);
	r.return_val = (void *)ia;
    }

    return r;
}

return_type concatStr(const int nparams, arg_type *a) {
    /* Concatenates 5 strings and returns them.
     * We assume a is 5 null-terminated strings. */

    if(nparams != 5) {
	r.return_val = NULL;
	r.return_size = 0;
	return r;
    }

    /* else */
    char *s[5];
    int i;
    int totallen = 0;

    for(i = 0; i < 5; i++) {
	s[i] = a->arg_val;
	a = a->next;
	totallen += strlen(s[i]);
    }

    r.return_val = (void *)calloc(totallen + 1, sizeof(char));
    ((char *)(r.return_val))[totallen] = (char) 0;

    for(i = totallen = 0; i < 5; totallen += strlen(s[i++])) {
	memcpy(((char *)(r.return_val)) + totallen, s[i], strlen(s[i]));
    }

    r.return_size = strlen((char *)(r.return_val)) + 1;
    return r;
}

int main() {
    register_procedure("nothing", 0, nothing);
    register_procedure("addtwo", 2, add);
    register_procedure("pickFirst", 2, pickFirst);
    register_procedure("max_of_integer_array", 1, max);
    register_procedure("concatenate_five_strings", 5, concatStr);

#ifdef _DEBUG_1_
    printRegisteredProcedures();
#endif

    launch_server();
    return 0;
}
