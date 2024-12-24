#ifndef DUMALLOC_H
#define DUMALLOC_H

#define FIRST_FIT 0
#define BEST_FIT 1

#define Managed(p) (*p)
#define Managed_t(t) t*

// The interface for managed DU malloc and free
void duManagedInitMalloc(int strategy);
void** duManagedMalloc(int size);
void duManagedFree(void** mptr);

void duMemoryDump();
void minorCollection();
void majorCollection();

#endif