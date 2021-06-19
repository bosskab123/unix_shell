#ifndef CHILD_INCLUDED
#define CHILD_INCLUDED

void ChildPID_terminate_handler(int iSig);

DynArray_T ChildPID_init(int size);

int ChildPID_getLength(DynArray_T cp);

int ChildPID_get(DynArray_T cp, int index);

void ChildPID_add(DynArray_T cp, int pid);

void ChildPID_delete(DynArray_T cp, int pid);

int ChildPID_compare(const void *pid1, const void *pid2);

void ChildPID_free(void *pvItem, void *pvExtra);

#endif
