#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dumalloc.h"

//original non managed prototypes
void duInitMalloc(int strategy);
void* duMalloc(int size, int heapNumber);
void duFree(void* ptr);

#define HEAP_SIZE (128*8)

#define NUM_HEAPS 3

//make a 2d array for our two heaps
unsigned char heap[NUM_HEAPS][HEAP_SIZE];
int allocationStrategy;
int currentHeap = 0; //current heap we are using

//if next is NULL, we are at the end of the list. 
typedef struct memoryBlockHeader {
    int free; // 0 - used, 1 = free
    int size; // size of the reserved block
    int managedIndex; // the unchanging index in the managed array
    int survivalAmt; // the number of times the block has moved between young heaps
    struct memoryBlockHeader* next;
} memoryBlockHeader;

//make a seperate free list head for each heap
memoryBlockHeader* freeListHead[NUM_HEAPS];

#define MANAGED_LIST_SIZE (HEAP_SIZE/8)
void* managedList[MANAGED_LIST_SIZE];
int managedListSize = 0;

void duManagedInitMalloc(int strategy){
    //call the original duInitMalloc
    duInitMalloc(strategy);
    //initialize all the managed pointers to NULL
    for (int i = 0; i < MANAGED_LIST_SIZE; i++){
        managedList[i] = NULL;
    }
}

void** duManagedMalloc(int size){
    //call the original duMalloc
    void* ptr = duMalloc(size, currentHeap);
    //if the pointer is null, meaning the call failed, return null
    if (ptr == NULL){
        return NULL;
    }
    //do pointer arithmetic to get the actual header of the block
    memoryBlockHeader* header = (memoryBlockHeader*) ((unsigned char*) ptr - sizeof(memoryBlockHeader));
    //set the appropriate index
    header->managedIndex = managedListSize;
    //set the pointer in the managed list
    managedList[managedListSize] = ptr;
    //increment the size
    managedListSize++;
    //return the pointer to the pointer
    return &managedList[managedListSize-1];
}

void duManagedFree(void** mptr){
    //dereference the pointer to get the actual pointer
    unsigned char* ptr = *mptr;
    //call the original duFree
    duFree(ptr);
    //set the pointer to null
    *mptr = NULL;
}

void duInitMalloc(int strategy){
    allocationStrategy = strategy;
    //initialize heap to 0
    for (int i = 0; i < HEAP_SIZE; i++){
        //initialize both heaps to 0
        heap[0][i] = 0;
        heap[1][i] = 0;
        heap[2][i] = 0;
    }

    //initialize all heaps
    memoryBlockHeader* currentBlock = (memoryBlockHeader*) heap[0];
    currentBlock->free = 1; //initialize to free
    currentBlock->size = HEAP_SIZE - sizeof(memoryBlockHeader);
    currentBlock->next = NULL; //no next block yet
    currentBlock->survivalAmt = 0; //initialize to 0
    freeListHead[0] = currentBlock;

    memoryBlockHeader* secondHeapBlock = (memoryBlockHeader*)heap[1];
    secondHeapBlock->free = 1;
    secondHeapBlock->size = HEAP_SIZE - sizeof(memoryBlockHeader);
    secondHeapBlock->next = NULL;
    secondHeapBlock->survivalAmt = 0;
    freeListHead[1] = secondHeapBlock;

    memoryBlockHeader* oldHeapBlock = (memoryBlockHeader*)heap[2];
    oldHeapBlock->free = 1;
    oldHeapBlock->size = HEAP_SIZE - sizeof(memoryBlockHeader);
    oldHeapBlock->next = NULL;
    oldHeapBlock->survivalAmt = 0;
    freeListHead[2] = oldHeapBlock;
}

void printMemoryBlockInfo(int heapIndex) {
    printf("Memory Block\n");
    //make a pointer to the start of all the memory
    memoryBlockHeader* current = (memoryBlockHeader*) heap[heapIndex];
    //make a string to represent the memory, need the plus 1 for the null terminator
    char memoryRepresentation[HEAP_SIZE/8+1];
    memoryRepresentation[HEAP_SIZE/8] = '\0';

    char freeChar = 'a';
    char usedChar = 'A';

    //iterate through the entire heap, which is the heap(pointer) + the size of the heap
     while ((unsigned char*)current < heap[heapIndex] + HEAP_SIZE) {
        if (current->free) {
            printf("Free at %p, size %d\n", current, current->size);
            //loop through the size of the entire block including the header
            for (int i = 0; i < (current->size + sizeof(memoryBlockHeader)) / 8; i++) {
                //to get the correct index for the array, we need to subtract the address of the heap from our current pointer so that we stay in bounds
                memoryRepresentation[((unsigned char*)current - heap[heapIndex]) / 8 + i] = freeChar;
            }
            freeChar++;
        } else {
            //similar to above, but for used blocks
            printf("Used at %p, size %d, managedIndex %d, survivalAmt %d\n", current, current->size, current->managedIndex, current->survivalAmt);
            for (int i = 0; i < (current->size + sizeof(memoryBlockHeader)) / 8; i++) {
                memoryRepresentation[((unsigned char*)current - heap[heapIndex]) / 8 + i] = usedChar;
            }
            usedChar++;
        }
        //manually go to the next block in memory since a free block won't point to a used block after it
        current = (memoryBlockHeader*)((unsigned char*)current + current->size + sizeof(memoryBlockHeader));
    }
    printf("%s\n", memoryRepresentation);
}

void printFreeList(int heapIndex) {
    printf("Free list\n");
    //make a copy so that we do not lose the free list head
    memoryBlockHeader* current = freeListHead[heapIndex];
    while (current != NULL) {
        printf("Block at %p, size %d\n", current, current->size);
        current = current->next;
    }
}

void printManagedList() {
    printf("Managed List\n");
    for (int i = 0; i < managedListSize; i++) {
        printf("ManagedList[%d] = %p\n", i, managedList[i]);
    }
}

void duMemoryDump() {
    printf("MEMORY DUMP\n");
    printf("Current heap (0/1 young): %d\n", currentHeap);
    printf("Young Heap (only the current one)\n");
    printMemoryBlockInfo(currentHeap);
    printFreeList(currentHeap);

    printf("Old Heap\n");
    printMemoryBlockInfo(2);
    printFreeList(2);
    printManagedList();
}

void* duMalloc(int size, int heapNumber){
    //get the remainder, if if is not 0, add 8-remainder to the original size, effectively rounding up to next multiple of 8
    int remainder = size % 8;
    //make copy of size to adjust for rounding
    int allignedSize = size;
    if (remainder != 0){
        allignedSize = allignedSize + (8-remainder);
    }

    int totalSizeNeeded = allignedSize + sizeof(memoryBlockHeader);

    //searching free list, make a copy of the header so we don't lose it
    memoryBlockHeader* current = freeListHead[heapNumber];
    //make a previous pointer to help with removal and special case
    memoryBlockHeader* prev = NULL;
    memoryBlockHeader* bestFit = NULL;
    memoryBlockHeader* bestFitPrev = NULL;
    
    if (allocationStrategy == FIRST_FIT){
        //walk through the entire list
        while (current != NULL){
            //check if the current block has enough space and that it is indeed free
            if (current->free && current->size >= totalSizeNeeded){
                //found suitable block
                break;
            }
            prev = current;
            current = current->next;
        }
    }
    else if (allocationStrategy == BEST_FIT){
        //walk through the entire list
        while (current != NULL){
            //check if the current block has enough space and that it is indeed free
            if (current->free && current->size >= totalSizeNeeded){
                //check if the best fit is null, if it is, set it to the current block
                if (bestFit == NULL){
                    bestFit = current;
                    bestFitPrev = prev;
                }
                //otherwise, check if the current block is smaller than the best fit, if it is, set the best fit to the current block
                else if (current->size < bestFit->size){
                    bestFit = current;
                    bestFitPrev = prev;
                }
            }
            prev = current;
            current = current->next;
        }
        //if we get to the end and still find nothing, set current to best fit
        current = bestFit;
        prev = bestFitPrev;
    }
    //if we get to end and still find nothing, return null
    if (current == NULL){
        return NULL;
    }
    //now we've found our suitable block, first we need to assign the header for the next free block. cast to a memoryBlockHeader pointer
    memoryBlockHeader* newBlock = (memoryBlockHeader*) ((unsigned char*) current + totalSizeNeeded);
    //adjust the size of the newblock and set to free
    newBlock->free = 1;
    newBlock->size = (current->size) - totalSizeNeeded;
    newBlock->survivalAmt = 0; //SUPER IMPORTANT, WE CANNOT ASSUME THINGS START AS 0
    newBlock->next = current->next;
    //then, adjust the size of the current block and set to not free
    current->size = allignedSize;
    current->free = 0;
    current->survivalAmt = 0;
    //remove the current block from the free list, but handle special case where the free block is the header of the free list, the new block needs to become new head
    if (current == freeListHead[heapNumber]){
        freeListHead[heapNumber] = newBlock;
        //we need to return here to prevent trying to access the next attribute of something NULL
        return (unsigned char*)current + sizeof(memoryBlockHeader);
    }
    //remove current from free list
    prev->next = newBlock; //removes reference to current, skipping over it
    current->next = NULL;
    //return memory address right under the current block's header
    return (unsigned char*)current + sizeof(memoryBlockHeader);
}

void duFree(void* ptr){
    //first, find location of the header
    memoryBlockHeader* header = (memoryBlockHeader*) ((unsigned char*) ptr - sizeof(memoryBlockHeader));
    header->free = 1; 
    header->survivalAmt = 0; //SUPER IMPORTANT, WE CANNOT ASSUME THINGS START AS 0

    //determine which heap we got a pointer to
    if ((unsigned char*)ptr >= heap[0] && (unsigned char*)ptr < heap[0] + HEAP_SIZE) {
        //traverse the list, we need a current pointer and a prev for linking
        memoryBlockHeader* current = freeListHead[0];
        memoryBlockHeader* prev = NULL;
        //iterate while the current (which is a mem address) is less than the header
        //once it is bigger, we will stop, and we know header needs to go before current
        while (current != NULL && current < header){
            prev = current;
            current = current->next;
        }

        //if prev is null, that means the header needs to be the head of the free list
        if (prev != NULL){
            //prev's next should be the header
            prev->next = header;
        }
        else{
            //otherwise, the header needs to become the head of the free list
            freeListHead[0] = header;
        }

        //after dealing with both cases, we still need to correct the header's next to link to rest of the list
        header->next = current;
    }
    //same as above
    else if ((unsigned char*)ptr >= heap[1] && (unsigned char*)ptr < heap[1] + HEAP_SIZE) {
        memoryBlockHeader* current = freeListHead[1];
        memoryBlockHeader* prev = NULL;
        while (current != NULL && current < header){
            prev = current;
            current = current->next;
        }
        if (prev != NULL){
            prev->next = header;
        }
        else{
            freeListHead[1] = header;
        }

        header->next = current;
    }
    //finally the old heap
    else if ((unsigned char*)ptr >= heap[2] && (unsigned char*)ptr < heap[2] + HEAP_SIZE) {
        memoryBlockHeader* current = freeListHead[2];
        memoryBlockHeader* prev = NULL;
        while (current != NULL && current < header){
            prev = current;
            current = current->next;
        }
        if (prev != NULL){
            prev->next = header;
        }
        else{
            freeListHead[2] = header;
        }

        header->next = current;
    }
}

void minorCollection(){
    int newHeap = 1 - currentHeap; 
    unsigned char* newHeapPtr = heap[newHeap]; 
    memoryBlockHeader* newFreeListHead = NULL;

    //make a pointer to the start of the heap that we can use for traversing.
    //we will want to be able to have access to the beginning of the heap later, which is why we do this in two different steps
    unsigned char* currentNewHeapPtr = newHeapPtr;

    // Move live objects
    for (int i = 0; i < managedListSize; i++) {
        //if it is alive we will move it
        if (managedList[i] != NULL) {
            //pointer arithmetic to get location of header
            memoryBlockHeader* oldHeader = (memoryBlockHeader*)((unsigned char*)managedList[i] - sizeof(memoryBlockHeader));
            //we are moving it, so its survived a collection
            oldHeader->survivalAmt++;
            if (oldHeader->survivalAmt >= 3) {
                //move to the old heap with malloc
                void* oldGenHeapPtr = duMalloc(oldHeader->size, 2); // Allocate in old heap
                if (oldGenHeapPtr == NULL) {
                    printf("Failed to allocate in old heap\n");
                    return;
                }
                //get actual header
                void* oldGenHeapHeader = (unsigned char*)oldGenHeapPtr - sizeof(memoryBlockHeader);

                memcpy(oldGenHeapHeader, oldHeader, oldHeader->size);
                managedList[i] = oldGenHeapPtr;
            }
            else{
                int blockSize = sizeof(memoryBlockHeader) + oldHeader->size;

                //copy block to other young heap
                memcpy(currentNewHeapPtr, oldHeader, blockSize);
                //update managed list correctly
                managedList[i] = currentNewHeapPtr + sizeof(memoryBlockHeader);
                //move to next position in new heap
                currentNewHeapPtr += blockSize;
            }
        }
    }

    // Create a free block for the remaining space in the new heap (if there is room)
    if (currentNewHeapPtr < newHeapPtr + HEAP_SIZE) {
        memoryBlockHeader* freeBlock = (memoryBlockHeader*)currentNewHeapPtr; 
        freeBlock->free = 1;
        //total size minus the size of the block, also accounting for size of the header
        freeBlock->size = (newHeapPtr + HEAP_SIZE) - currentNewHeapPtr - sizeof(memoryBlockHeader);
        freeBlock->next = NULL;
        freeBlock->survivalAmt = 0; //SUPER IMPORTANT, WE CANNOT ASSUME THINGS START AS 0
        //update the free list head
        newFreeListHead = freeBlock;
    }

    // Update the overall free list to be the new one
    freeListHead[newHeap] = newFreeListHead;

    // Switch the current heap to the new heap
    currentHeap = newHeap;
}

void majorCollection() {
    // Pointer to the start of the old heap
    unsigned char* oldHeapPtr = heap[2];
    
    // Pointer to the first block in the old heap
    memoryBlockHeader* current = (memoryBlockHeader*)oldHeapPtr;
    
    // Pointer to the first free block in the old heap
    memoryBlockHeader* firstFree = NULL;
    memoryBlockHeader* prevFree = NULL;

    // Iterate through the old heap
    while ((unsigned char*)current < oldHeapPtr + HEAP_SIZE) {
        if (current->free) {
            // If we find a free block and firstFree is NULL, this is the first free block
            if (firstFree == NULL) {
                firstFree = current;
                prevFree = NULL;
            } else {
                // Coalesce free blocks that are adjacent by adjusting the pointer (smushes them together)
                if (prevFree != NULL) {
                    prevFree->next = current->next;
                }
                //then adjust the size to be correct
                firstFree->size += current->size + sizeof(memoryBlockHeader);
            }
        } else {
            // If we find a used block and there is a free block before it (not if we haven't found the first free block yet)
            if (firstFree != NULL) {
                //save the information that is found in the first free block (size and next)
                int firstFreeSize = firstFree->size;
                struct memoryBlockHeader* firstFreeNext = firstFree->next;

                //convert the first free into the used block by updating header information
                firstFree->size = current->size;
                firstFree->next = current->next;
                firstFree->free = 0;
                firstFree->survivalAmt = current->survivalAmt;
                firstFree->managedIndex = current->managedIndex;

                //memcpy only the data 
                memcpy((unsigned char*)firstFree + sizeof(memoryBlockHeader), (unsigned char*)current + sizeof(memoryBlockHeader), current->size);
                
                // Update the managed list to point to the new location
                managedList[firstFree->managedIndex] = (unsigned char*)firstFree + sizeof(memoryBlockHeader); //old memory will be placed on top of first free pointer

                //now change first free's size and next back to what they were
                firstFree = (memoryBlockHeader*)((unsigned char*)firstFree + firstFree->size + sizeof(memoryBlockHeader));
                firstFree->size = firstFreeSize;
                firstFree->next = firstFreeNext;
                firstFree->free = 1;
                firstFree->survivalAmt = 0;  
            }
        }
        // Move to the next block in the old heap
        current = (memoryBlockHeader*)((unsigned char*)current + current->size + sizeof(memoryBlockHeader));
    }
    // Update the free list head for the old heap
    freeListHead[2] = firstFree;
    //set to null when done with everything to prevent printing an extra free block
    freeListHead[2]->next = NULL;
}
