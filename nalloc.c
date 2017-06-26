/*
 * nalloc.c
 *
 * The neil memory allocator.
 *
 * Neil Klingensmith
 * 17 August 2012
 *
 * This file contains C code for a memory allocator for embedded systems.
 * The allocator uses a first-fit policy to find suitable memory blocks.
 * Block lists are maintained in address order. The heap is fixed-size
 * and allocated at compile time.
 *
 * License.
 *
 * This software is in the public domain. It may be freely redistributed.
 *
 */

/*
 * Notes
 *
 *
 * 2013-05-07 NAK
 *   HEAPSIZE changed to CONFIG_HEAP_SIZE, and definition removed from
 *   nalloc.h. CONFIG_HEAP_SIZE must be defined on the command line by passing
 *   -DCONFIG_HEAP_SIZE=xxx as a parameter to gcc when compiling.
 *
 *
 *
 */







#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Number of bytes to round all allocations to
#define ALIGNSIZE 2

// Size of the heap
#define CONFIG_HEAP_SIZE 2048

struct block
{
	struct block *next ; // Points to the next element in the linked list
	struct block *prev ; // Points to the previous element in the linked list
	int size ;           // Size of the block + memory
} ;

char heap[CONFIG_HEAP_SIZE] ;

struct block *freeListHead = NULL, *allocatedListHead = NULL ;

/*
 * listInsert
 * 
 * Inserts a block structure b into a list pointed to by listHead.
 * The lists are sorted by starting address of the block.
 *
 */
void listInsert(struct block **listHead, struct block *b)
{
	struct block *iterator = (struct block*)listHead ;

	// Step through the list until iterator points to the last
	// element in the list that is at a lower address than b.
	while(iterator->next != NULL && b > iterator->next)
		iterator = iterator->next ;

	// Link element b into the list between iterator and iterator->next.
	b->next = iterator->next ;
	b->prev = iterator ;

	iterator->next = b ;
	
	if(b->next != NULL)
		b->next->prev = b ;
}


/*
 * listDelete
 *
 * Deletes an element from a doubly linked list.
 */
void listDelete(struct block *b)
{
	if(b->next != NULL)
		b->next->prev = b->prev ;
	
	b->prev->next = b->next ;

	// NULLify the element's next and prev pointers to indicate
	// that it is not linked into a list.
	b->next = NULL ;
	b->prev = NULL ;
}


/*
 * memInit
 *
 * Initializes the heap by creating a block structure at the beginning
 * and linking it into the free list.
 *
 */
void memInit()
{
	freeListHead = NULL;
	allocatedListHead = NULL ;
	struct block *temp = (struct block*)heap ;

	// Insert the heap into the free list.
	listInsert(&freeListHead, temp) ;
	
	((struct block*)temp)->size = CONFIG_HEAP_SIZE - sizeof(struct block);
}


/*
 * nalloc
 *
 * Returns a pointer to a memory block of lenght size. If no block can
 * be found to satisfy the request, returns NULL.
 *
 */
void *nalloc(int size)
{
	struct block *iterator = freeListHead ;
	struct block *newBlock = NULL ;

	// Reserve space for a block structure
	size += sizeof(struct block) ;

	// Step through the free list looking for blocks that are
	// large enough to fulfill the request.
	while(iterator != NULL && iterator->size < size)
		iterator = iterator->next ;

	// If we stepped through the entire list without finding
	// anything, return NULL.
	if(iterator == NULL)
		return NULL ;

	// Split the block up into two blocks if there is room in to
	// store a second block structure in the space not needed by
	// this allocation. Otherwise, return the full block.
	if((iterator->size - size) > sizeof(struct block))
	{
		// Split the block up.
		newBlock = (struct block*)((long)iterator + size) ;
		newBlock->size = iterator->size - size ;
		iterator->size -= newBlock->size ;
		
		// Link the new block in to the free list.
		listInsert(&freeListHead, newBlock) ;
	}

	// Delete iterator from the free list and insert it into the allocated list.
	listDelete(iterator) ;

	// Insert the iterator into the allocated list.
	listInsert(&allocatedListHead, iterator) ;

	return (void*)((long)iterator + sizeof(struct block)) ;
}

/*
 * nfree
 *
 * Frees a block of memory by unlinking it from the allocated list
 * and linking it in to the free list.
 */
void nfree(void *block)
{
	struct block *b = (struct block*)((long)block - sizeof(struct block)) ;

	// Delete the block from the active list
	listDelete(b) ;

	// Insert the block into the free list
	listInsert(&freeListHead, b) ;

	// Check if we can coalesce with the next block.
	if((long)b + b->size == (long)b->next)
	{
		// Add the size of b to the next block's size.
		b->size += b->next->size ;

		// Delete b from the free list.
		listDelete(b->next) ;
	}

	// Check if we can coalsece with the previous block.
	if((b->prev != (struct block*)&freeListHead) && ((long)b->prev + b->prev->size) == (long)b)
	{
		// Add the size of b to the size of the previous block
		b->prev->size += b->size ;

		// Delete b from the free list.
		listDelete(b) ;
	}
}

