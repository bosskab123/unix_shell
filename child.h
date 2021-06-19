#ifndef CHILD_INCLUDED
#define CHILD_INCLUDED

enum ProcessType { PROCESS_BG, PROCESS_FG };

void freeProcess(void *pvItem, void *pvExtra);

enum ProcessType Process_getType(void *pvItem);

int Process_getpid(void *pvItem);

struct Token *makeProcess(enum ProcessType eProcessType, int pid);

DynArray_T Process_init(int size);

int Process_getLastbg(DynArray_T p);

void Process_terminate(DynArray_T p, int pid);

int Process_getIndex(DynArray_T p, int pid);

// DynArray_T ChildPID_init(int size);

// int ChildPID_getLength(DynArray_T cp);

// int ChildPID_get(DynArray_T cp, int index);

// void ChildPID_add(DynArray_T cp, int pid);

// void ChildPID_delete(DynArray_T cp, int pid);

// int ChildPID_compare(const void *pid1, const void *pid2);

// void ChildPID_free(void *pvItem, void *pvExtra);

#endif
