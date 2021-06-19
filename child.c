#include <stdio.h>
#include <assert.h>
#include "dynarray.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>



void ChildPID_terminate_handler(int iSig)
{
	wait(NULL);
}

DynArray_T ChildPID_init(int size)
{
	assert(size >= 0);
	DynArray_T cp = DynArray_new(size);
	if (cp == NULL)
	{
		fprintf(stderr, "Cannot allocate memory\n");
		exit(EXIT_FAILURE);
	}
	return cp;
}

int ChildPID_getLength(DynArray_T cp)
{
	assert(cp != NULL);
	return DynArray_getLength(cp);
}

int ChildPID_get(DynArray_T cp, int index)
{
	assert(cp != NULL);
	int *item;
	item = DynArray_get(cp,index);
	return *item;
}

void ChildPID_add(DynArray_T cp, int pid)
{
	assert(cp != NULL);
	int *p = (int *)malloc(sizeof(int));
	*p = pid;
	DynArray_add(cp, p);
}

void ChildPID_delete(DynArray_T cp, int pid)
{
	assert(cp!=NULL);
	int i;
	int length = DynArray_getLength(cp);
	for(i=0;i<length;i++){
		if(ChildPID_get(cp,i) == pid) {
			DynArray_removeAt(cp,i);
			return;
		}
	}
}

int ChildPID_compare(const void *pid1, const void *pid2)
{
	assert(pid1 != NULL);
	assert(pid2 != NULL);
	return (int *)pid1 == (int *)pid2;
}

void ChildPID_free(void *pvItem, void *pvExtra)
{
	assert(pvItem != NULL);
	int *childPID = (int *)pvItem;
	free(childPID);
}
