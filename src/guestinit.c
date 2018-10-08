
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



#include "hermes.h"

/////////////////////////////////////////////////////
// Global variables
extern struct interrupt intlist[NUM_PERIPHERAL_INTERRUPTS];


// List of guests running on Hermes
struct vm *__dummyguest;



/*
 * ioInit
 *
 * This is where we assign MCU peripherals to guests in Hermes. For each
 * peripheral, we are going to do three basic tasks:
 *
 *    1. Set the interrupt's priority in the ARM's Nonvolatile Interrupt
 *       Controller (NVIC) Interrupt Priority Register (IPR)
 *    2. Enable interrupt source in the Interrupt Set Enable Register (ISER).
 *    3. Assign the interrupt to a guest in the in Hermes's intlist[] array.
 */
void ioInit(void) {
    
    // Set interrupt priority in the IPR
    NVIC_IPR[45] = 0xc0;// Set priority of UART3 interrupt to 0xc0
    
    // Enable interrupt in ISER
    ISER1 |= (1<<(45-32));// Enable UART3 interrupt in the NVIC
    
    // Assign interrupt source to guest
    intlist[45].owner = __dummyguest; // UART3
    intlist[45].priority = 0xc0; // UART3

}

/*
 * guestInit
 *
 * Initialize guests 
 *
 */
void guestInit(void) {
    extern void *dummyVectorTable;

    __dummyguest = createGuest(&dummyVectorTable);

}
