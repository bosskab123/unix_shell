#ifndef CHILD_INCLUDED
#define CHILD_INCLUDED

void ChildPID_terminate_handler(int iSig);

DynArray_T ChildPID_init(int size);

void ChildPID_free(void *pvItem, void *pvExtra);

void ChildPID_add(DynArray_T cp, int pid);

int ChildPID_compare(const void *pid1, const void *pid2);

#endif
