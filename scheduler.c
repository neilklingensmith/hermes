/*

MIT License

Copyright (c) 2018 Neil Klingensmith

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

///////////////////////////////////////////////
// Includes

#include <stdint.h>
#include <string.h>
#include "hermes.h"



///////////////////////////////////////////////
// Global Variables

struct vm *guestList = NULL, *sleepingList = NULL, *currGuest = NULL;



/*
 * listAdd
 *
 * Adds newElement to a linked list pointed to by list. When calling this function, pass the address of list head.
 *
 */
int listAdd(struct vm **head, struct vm *newElement){
	struct vm *iterator = (struct listElement*)head ;
	
	// Sort the list by BASEPRI
	while((iterator->next->BASEPRI <= newElement->BASEPRI) /*&& (iterator->next->BASEPRI != 0)*/ && (iterator->next != NULL)){
		iterator = iterator->next;
	}

	// Link element b into the list between iterator and iterator->next.
	newElement->next = iterator->next ;
	newElement->prev = iterator ;

	iterator->next = newElement ;
	
	if(newElement->next != NULL){
		newElement->next->prev = newElement ;
	}
	return 0;
}


/*
 * listRemove
 *
 * Deletes an element from a doubly linked list.
 */
void listRemove(struct listElement *element)
{
	if(element->next != NULL)
		element->next->prev = element->prev ;
	
	element->prev->next = element->next ;

	// NULLify the element's next and prev pointers to indicate
	// that it is not linked into a list.
	element->next = NULL ;
	element->prev = NULL ;
}



int hvScheduler(uint32_t *psp){
	// Save the current guest's context.
	if(GUEST_IN_MASTER_MODE(currGuest)){
		currGuest->MSP = psp;
	} else {
		currGuest->PSP = psp;
	}

#if 0
	// Dumb scheduler. Cycle thru guests.
	if(currGuest->next != NULL){
		currGuest = currGuest->next;
	} else {
		currGuest = guestList;
	}
#endif

	if((guestList->next->next!= NULL) && (guestList->next->BASEPRI == guestList->next->next->BASEPRI)){
		struct vm *topGuest = guestList;
		listRemove(topGuest);
		listAdd(&guestList,topGuest);
	}

	currGuest = guestList;
	
	
	// Install the new guest's PSP
	if(GUEST_IN_MASTER_MODE(currGuest)){
		SET_CPU_PSP(currGuest->MSP);
	} else {
		SET_CPU_PSP(currGuest->PSP);
	}

	if(GET_PROCESSOR_EXCEPTION(currGuest) != 0){
		__asm volatile(
		//		"mrs %0,basepri\n" // Save the guest's BASEPRI
		"movs r0, 0xff\n"  // set the guest's BASEPRI to 0xff to disable interrupts while we're running the guest's ISR
		"msr basepri,r0\n"
		://"=r"(currGuest->BASEPRI) // output
		:                         // input
		:"r0"                     // clobbered register
		);
		} else {
		SET_CPU_BASEPRI(currGuest->BASEPRI);
	}
	
	return 0;
}


