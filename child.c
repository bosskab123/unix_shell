#include <stdio.h>
#include "dynarray.h"
#include <stdlib.h>
#include <unistd.h>
#include<sys/wait.h>

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

int ChildPID_compare(void *pid1, void *pid2)
{
	return (int)*pid1 == (int)*pid2;
}
