#include <stdio.h>
#include "dynarray.h"
#include <stdlib.h>

void ChildPID_terminate_handler(int iSig)
{
	wait(NULL);
}

void ChildPID_free(void *pvItem, void *pvExtra)
{
	int *childPID = (int *)pvItem;
	free(childPID);
}

DynArray_T ChildPID_init(int size)
{
	DynArray_T
	cp = DynArray_new(size);
	if (cp == NULL)
	{
		fprintf(stderr, "Cannot allocate memory\n");
		exit(EXIT_FAILURE);
	}
	return cp;
}

void ChildPID_add(DynArray_T cp, int pid)
{
	int *child_pid;
	child_pid = (int *)malloc(sizeof(int));
	*child_pid = pid;
	DynArray_add(cp, child_pid);
}
