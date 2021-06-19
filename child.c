#include <stdio.h>
#include <assert.h>
#include "dynarray.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

enum ProcessType { PROCESS_BG, PROCESS_FG, PROCESS_TERMINATED };

struct Process
{
	/* The type of process */
	enum ProcessType pType;

	/* process ID */
	int pid;
}

void freeProcess(void *pvItem, void *pvExtra)

/* Free token pvItem.  pvExtra is unused. */

{
	assert(pvItem != NULL);
	struct Process *psProcess = (struct Process *)pvItem;
	free(psProcess);
}

enum ProcessType Process_getType(void *pvItem)

/* Return type to caller */

{
	assert(pvItem != NULL);
	struct Process *psProcecss = (struct Process*)pvItem;
	return psProcess->pType;
}

int Process_getpid(void *pvItem)

/* Return pid to caller */

{
	assert(pvItem != NULL);
	struct Process *psProcess = (struct Process *)pvItem;
	return psProcess->pid;
}

struct Process *makeProcess(enum ProcessType eProcessType, int pid)

/* Create and return a Process whose type is eProcessType and whose
   value is pid.  Return NULL if insufficient
   memory is available.  The caller owns the Process. */

{
	assert(pid > 0);
	struct Process *psProcess;

	psProcess = (struct Process *)malloc(sizeof(struct Process));
	if (psProcess == NULL)
    	return NULL;

	psProcess->pType = eProcessType;

	psProcess->pid = pid;
	return psProcess;
}

/* Initiate an array of process running in the shell */
DynArray_T Process_init(int size)
{
	assert(size >= 0);
	DynArray_T p = DynArray_new(size);
	if (p == NULL)
	{
		fprintf(stderr, "Cannot allocate memory\n");
		exit(EXIT_FAILURE);
	}
	return p;
}

/* Get the last process running in background */
int Process_getLastbg(DynArray_T p)
{
	assert(p != NULL);
	int length = DynArray_getLength(p);
	int i;
	for(i=length-1;i>=0;i--){
		if(Process_getType(DynArray_get(p,i)) == PROCESS_BG){
			return Process_getpid(DynArray_get(p,i));
		} 
	}
	return -1;
}

void Process_terminate(DynArray_T p, int pid)
{
	assert(p != NULL);
	assert(pid > 0);
	int length = DynArray_getLength(p);
	int i;
	for(i=length-1;i>=0;i--){
		if(Process_getpid(DynArray_get(p,i)) == pid){
			struct Process *psProcess = (struct Process *)DynArray_get(p,i);
			psProcess->pType = PROCESS_TERMINATED;
			return;
		} 
	}
}

void Process_add(DynArray_T p, int pid, enum ProcessType eProcessType)
{
	assert(p != NULL);
	assert(pid > 0);
	struct Process *psProcess;
	psProcess = makeProcess(eProcessType, pid);
	if(psProcess == NULL)
	{
		fprintf(stderr, "Cannot allocate memory\n");
		exit(EXIT_FAILURE);
	} 

	DynArray_add(p, psProcess);
}
// DynArray_T ChildPID_init(int size)
// {
// 	assert(size >= 0);
// 	DynArray_T cp = DynArray_new(size);
// 	if (cp == NULL)
// 	{
// 		fprintf(stderr, "Cannot allocate memory\n");
// 		exit(EXIT_FAILURE);
// 	}
// 	return cp;
// }

// int ChildPID_getLength(DynArray_T cp)
// {
// 	assert(cp != NULL);
// 	return DynArray_getLength(cp);
// }

// int ChildPID_get(DynArray_T cp, int index)
// {
// 	assert(cp != NULL);
// 	int *item;
// 	item = DynArray_get(cp,index);
// 	return *item;
// }

// void ChildPID_add(DynArray_T cp, int pid)
// {
// 	assert(cp != NULL);
// 	int *p = (int *)malloc(sizeof(int));
// 	*p = pid;
// 	DynArray_add(cp, p);
// }

// void ChildPID_delete(DynArray_T cp, int pid)
// {
// 	assert(cp!=NULL);
// 	int i;
// 	int length = DynArray_getLength(cp);
// 	for(i=0;i<length;i++){
// 		if(ChildPID_get(cp,i) == pid) {
// 			DynArray_removeAt(cp,i);
// 			return;
// 		}
// 	}
// }

// int ChildPID_compare(const void *pid1, const void *pid2)
// {
// 	assert(pid1 != NULL);
// 	assert(pid2 != NULL);
// 	return (int *)pid1 == (int *)pid2;
// }

// void ChildPID_free(void *pvItem, void *pvExtra)
// {
// 	assert(pvItem != NULL);
// 	int *childPID = (int *)pvItem;
// 	free(childPID);
// }
