
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
#include <string.h>
#include <stdlib.h>




///////////////////////////////////////////////
// Global Variables


char hvStack[HV_STACK_SIZE]; // ISR Stack for Hermes
void *ramVectors[80]  __attribute__ ((aligned (128))); // RAM-located vector table for Hermes
struct interrupt intlist[NUM_PERIPHERAL_INTERRUPTS];


extern struct vm *guestList, *sleepingList, *currGuest; // Defined in the scheduler


__attribute__ ((section(".vectors")))
void *hvVectorTable[] __attribute__ ((aligned (128))) = {
    /* Configure Initial Stack Pointer, using linker-generated symbols */
    (void*) (void*) (&hvStack[HV_STACK_SIZE]-4),/* Initial stack pointer */
    (void*) hermesResetHandler,/* Reset Handler */
    (void*) genericHandler,    /* NMI Handler */
    (void*) genericHandler,    /* Hard Fault Handler */
    (void*) genericHandler,    /* MemManage Handler */
    (void*) genericHandler,    /* Bus Fault Manager */
    (void*) genericHandler,    /* Usage Fault Handler */
    (void*) genericHandler,    /* Reserved */
    (void*) genericHandler,    /* Reserved */
    (void*) genericHandler,    /* Reserved */
    (void*) genericHandler,    /* Reserved */
    (void*) genericHandler,    /* SVC Handler */
    (void*) genericHandler,    /* Debug Mon Handler */
    (void*) genericHandler,    /* Reserved */
    (void*) genericHandler,    /* PendSV Handler */
    (void*) genericHandler,    /* SysTick Handler */

    /* Configurable interrupts */
    (void*) genericHandler,    /* 0  Supply Controller */
    (void*) genericHandler,    /* 1  Reset Controller */
    (void*) genericHandler,    /* 2  Real Time Clock */
    (void*) genericHandler,    /* 3  Real Time Timer */
    (void*) genericHandler,    /* 4  Watchdog Timer */
    (void*) genericHandler,    /* 5  Power Management Controller */
    (void*) genericHandler,    /* 6  Enhanced Embedded Flash Controller */
    (void*) genericHandler,    /* 7  UART 0 */
    (void*) genericHandler,    /* 8  UART 1 */
    (void*) genericHandler,    /* 9  Reserved */
    (void*) genericHandler,    /* 10 Parallel I/O Controller A */
    (void*) genericHandler,    /* 11 Parallel I/O Controller B */
    (void*) genericHandler,    /* 12 Parallel I/O Controller C */
    (void*) genericHandler,    /* 13 USART 0 */
    (void*) genericHandler,    /* 14 USART 1 */
    (void*) genericHandler,    /* 15 USART 2 */
    (void*) genericHandler,    /* 16 Parallel I/O Controller D */
    (void*) genericHandler,    /* 17 Parallel I/O Controller E */
    (void*) genericHandler,    /* 18 Multimedia Card Interface */
    (void*) genericHandler,    /* 19 Two Wire Interface 0 HS */
    (void*) genericHandler,    /* 20 Two Wire Interface 1 HS */
    (void*) genericHandler,    /* 21 Serial Peripheral Interface 0 */
    (void*) genericHandler,    /* 22 Synchronous Serial Controller */
    (void*) genericHandler,    /* 23 Timer/Counter 0 */
    (void*) genericHandler,    /* 24 Timer/Counter 1 */
    (void*) genericHandler,    /* 25 Timer/Counter 2 */
    (void*) genericHandler,    /* 26 Timer/Counter 3 */
    (void*) genericHandler,    /* 27 Timer/Counter 4 */
    (void*) genericHandler,    /* 28 Timer/Counter 5 */
    (void*) genericHandler,    /* 29 Analog Front End 0 */
    (void*) genericHandler,    /* 30 Digital To Analog Converter */
    (void*) genericHandler,    /* 31 Pulse Width Modulation 0 */
    (void*) genericHandler,    /* 32 Integrity Check Monitor */
    (void*) genericHandler,    /* 33 Analog Comparator */
    (void*) genericHandler,    /* 34 USB Host / Device Controller */
    (void*) genericHandler,    /* 35 MCAN Controller 0 */
    (void*) genericHandler,    /* 36 Reserved */
    (void*) genericHandler,    /* 37 MCAN Controller 1 */
    (void*) genericHandler,    /* 38 Reserved */
    (void*) genericHandler,    /* 39 Ethernet MAC */
    (void*) genericHandler,    /* 40 Analog Front End 1 */
    (void*) genericHandler,    /* 41 Two Wire Interface 2 HS */
    (void*) genericHandler,    /* 42 Serial Peripheral Interface 1 */
    (void*) genericHandler,    /* 43 Quad I/O Serial Peripheral Interface */
    (void*) genericHandler,    /* 44 UART 2 */
    (void*) genericHandler,    /* 45 UART 3 */
    (void*) genericHandler,    /* 46 UART 4 */
    (void*) genericHandler,    /* 47 Timer/Counter 6 */
    (void*) genericHandler,    /* 48 Timer/Counter 7 */
    (void*) genericHandler,    /* 49 Timer/Counter 8 */
    (void*) genericHandler,    /* 50 Timer/Counter 9 */
    (void*) genericHandler,    /* 51 Timer/Counter 10 */
    (void*) genericHandler,    /* 52 Timer/Counter 11 */
    (void*) genericHandler,    /* 53 Reserved */
    (void*) genericHandler,    /* 54 Reserved */
    (void*) genericHandler,    /* 55 Reserved */
    (void*) genericHandler,    /* 56 AES */
    (void*) genericHandler,    /* 57 True Random Generator */
    (void*) genericHandler,    /* 58 DMA */
    (void*) genericHandler,    /* 59 Camera Interface */
    (void*) genericHandler,    /* 60 Pulse Width Modulation 1 */
    (void*) genericHandler,    /* 61 Reserved */
    (void*) genericHandler,    /* 62 SDRAM Controller */
    (void*) genericHandler     /* 63 Reinforced Secure Watchdog Timer */
};



// Initialize segments. These are defined in the linker script
extern uint32_t _sfixed;
extern uint32_t _efixed;
extern uint32_t _etext;
extern uint32_t _srelocate;
extern uint32_t _erelocate;
extern uint32_t _szero;
extern uint32_t _ezero;
extern uint32_t _sstack;
extern uint32_t _estack;

/*
 * hermesResetHandler
 *
 * Reset handler for Hermes.
 *
 *    1. Initializes the .bss and data segments of memory
 *
 *    2. Initializes hypervisor with hvInit()
 *
 *    3. Creates guests. Calls to createGuest() must come after hvInit().
 *
 *    4. Starts running guests.
 */
void hermesResetHandler(){
    //extern void *dummyVectorTable[];
    int i;
    uint8_t *pB;
    register uint32_t *pSrc, *pDest; // Must be register variables because if they are stored on the stack (which is in the .bss section), their values will be obliterated when clearing .bss below

    // Enable cycle count
    *((uint32_t*)0xe000edfc) |= 0x01000000; // Enable DWT
    *((uint32_t*)0xe0001004) = 0; // Reset cycle counter
    *((uint32_t*)0xE0001000) |= 1; // Enable cycle counter

    /* Initialize the relocate segment */
    pSrc = &_etext;
    pDest = &_srelocate;

    if (pSrc != pDest) {
        for (; pDest < &_erelocate;) {
            *pDest++ = *pSrc++;
        }
    }

    /* Clear the zero segment */
    for (pDest = &_szero; pDest < &_ezero;) {
        *pDest++ = 0;
    }
    
    // Put the exception table into RAM so we can modify it if needed.
    memcpy(ramVectors,hvVectorTable, sizeof(hvVectorTable));

    //CORTEXM7_VTOR = ((uint32_t) hvVectorTable);
    CORTEXM7_VTOR = ((uint32_t) ramVectors);

    hvInit();
    
    guestInit();

    // Set configurable interrupts to low priority
    pDest = (uint32_t *)0xe000e400;
    while(pDest < (uint32_t *)0xe000e500){
        *pDest = 0xffffffff;
        pDest++;
    }

    for(i = 0; i < NUM_PERIPHERAL_INTERRUPTS; i++){
        intlist[i].owner = NULL;
        intlist[i].priority = 0xe0;
    }

    // Hardware init
#if HERMES_ETHERNET_BRIDGE == 1
    /* Configure systick for 1 ms. */
    extern void TimeTick_Configure(void);
    TimeTick_Configure();
    
    // The Atmel ethernet driver uses the SysTick exception to time delays.
    // Since we haven't started the HV yet, we can't use the HV's exception
    // hooks to call Atmel's library SysTick Handler. Instead, we will
    // temporarily put the library SysTick handler into the exception table and
    // allow it to be called directly for purposes of driver initialization. We
    // then replace the hypervisor's hook when we're done with the init.
    void SysTick_Handler(void);
    ramVectors[15] = SysTick_Handler;

//    uint8_t macaddr[] = {0x3a, 0x1f, 0x34, 0x08, 0x54, 0x54};
    gmac_tapdev_init_hermes();
    CORTEXM7_SYST_CVR = 0xc0; // Reset the system timer so we don't get a SysTick exception before we start the HV
    ramVectors[15] = hvVectorTable[15];
#endif


    // Set interrupt enable registers to 1's. Only enable interrupts that are implemented on this chip.
    asm("cpsid i"); // Globally disable interrupts during setup.
    pB = (uint8_t*)0xe000e100;
    for(i = 0; i < NUM_PERIPHERAL_INTERRUPTS/8; i++){
        *pB++ = 0xff;
    }
    ioInit(); // Initialize interrupt sources and assign to guests (application specific)
//SCB_EnableICache();
//SCB_EnableDCache();


    // Initialize SysTick Module
    CORTEXM7_SYST_RVR = 4*0x249f0;
    CORTEXM7_SYST_CSR = 7;
    CORTEXM7_SYST_CVR = 0;

    // Switch to unpriv execution
    __asm volatile
    (
    "  msr psp,%0\n" // Put the currGuest's MSP into the PSP
    "  cpsie i\n" // Enable exceptions
    "  mrs r0,control\n" // Set control register to put CPU in thread mode, switching to PSP
    "  orr r0,r0,#3\n"
    "  msr control,r0\n"
    "  ldr lr, =0xfffffff1\n" // Put return to handler mode, non floating point state into guest's LR
    "  bx lr\n" // Return
        :                        /* output */
        : "r"(currGuest->MSP)    /* input */
        :                        /* clobbered register */
    );
}

