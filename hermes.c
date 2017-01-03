
#include "hermes.h"
#include <stdint.h>

char hvStack[HV_STACK_SIZE];

int **guestExceptionTable;

void hvInit(void *gET) {
	uint32_t oldMSP;
	uint32_t returnAddress;
	
	// Save regs & preserve MSP
	__asm volatile
	(
	"  push {r1-r12,r14}\n"
	"  mrs %0,msp\n"
	"  msr psp,%0\n"
	:                   /* output */
	:"r"(oldMSP) /* input */
	:                   /* clobbered register */
	);
	uint32_t stackTop = (uint32_t)hvStack+HV_STACK_SIZE-4*14;
	int *src = (int*)oldMSP, *dst = (int*)stackTop;
/*
	for(int k = 0; k < 14; k++){
		*dst++ = *src++;
	}
*/
	// Save return address
	__asm volatile
	(
	"  mov %0,r14\n"
	:"=r"(returnAddress) /* output */
	:                    /* input */
	:                    /* clobbered register */
	);
	
	// Copy register contents from old stack to new stack
	
	// Set up guest data structures.
	guestExceptionTable = gET;
	
	// Update main stack pointer with HV's top of stack.
	__asm volatile
	(
	"  msr msp,%0\n"
	:                   /* output */
	:"r"(stackTop) /* input */
	:                   /* clobbered register */
	);
	// Switch to unpriv execution
	__asm volatile
	(
	"  mrs r0,psp\n"      // Get PSP in R0
	"  ldm r0!,{r1-r12,r14}\n" // Get saved regs off PSP
	"  mrs r0,control\n"
	"  orr r0,r0,#3\n"
	"  msr control,r0\n"
	);
#if 0
	// Restore regs
	__asm volatile
	(
	"  mrs r0,psp\n"      // Get PSP in R0
//	"  ldm r0!,{r1-r12,r14}\n" // Get saved regs off PSP
	"  add r0,-32\n"      // Build an exception frame on the stack
	"  msr psp,r0\n"      // Update PSP
	"  str r0,[r0,#28]\n" // Put dummy xPSR on exception frame
	"  str %0,[r0,#24]\n" // Put the address of the guest exception vector on the stack frame--this is the return PC
	"  str lr,[r0,#20]\n" // Put the LR on the guest exception frame. XXX This needs to be changed--modify LR to always return to thread mode.
	"  str r12,[r0,#16]\n" // Zero-out the saved registers
	"  str r3,[r0,#12]\n"
	"  str r2,[r0,#8]\n"
	"  str r1,[r0,#4]\n"
	"  str r0,[r0,#0]\n"
	"  ldr lr, =0xfffffffd\n"  // Overwrite LR, forcing exception return to thread mode
	"  bx lr\n"
	:                   /* output */
	:"r"(returnAddress) /* input */
	:                   /* clobbered register */
	);
#endif
}

/*
 * getIPSR()
 *
 * Reads the value of the IPSR, which indicates what interrupt number caused
 * the current exception..
 */
uint32_t getIPSR() __attribute__((naked));
uint32_t getIPSR() {
	__asm("  mrs r0, IPSR\n"
	      "  bx lr\n");
}

/*
 * locateGuestISR(int interruptNum)
 *
 * Parses the guest's exception table, and returns the address of the guest's
 * exception handler associated with interruptNum, the exception number.
 */
int *locateGuestISR(int interruptNum){
	return (int*)guestExceptionTable[interruptNum]; // stub that returns function pointer to guest exception table.
}

/*
 * genericHandler()
 *
 * Hermes exception handler for all exceptions.
 */
#if 0
void genericHandler() __attribute__((naked));
void genericHandler() {

	__asm volatile
	(
	"  mov.w r3, #128\n" // Copied from FreeRTOS code
	"  cpsid i\n"
	"  msr basepri, r3\n"
	"  isb sy\n"           // Sync I$
	"  dsb sy\n"           // Sync D$
	"  cpsie i\n"
	"  push {lr}\n" // Save LR so we don't overwrite in function call.

	);

	int exceptionNum = getIPSR(); // Read IPSR to get exception number
	void *guestExceptionVector = locateGuestISR(exceptionNum); // Get the address of the guest's exception handler corresponding to the exception number


	switch(exceptionNum){
	case 2: // NMI
		break;
	case 3: // HardFault
		break;
	case 4: // MemManage
		break;
	case 5: // BusFault
		break;
	case 6: // UsageFault
		break;
	case 7:	 // Reserved
	case 8:
	case 9:
	case 10:
		break;
	case 11: // SVCall
		break;
	case 12: // Reserved for debug
	case 13: // Reserved
		break;
	case 14: // PendSV
		break;
	case 15: // SysTick
		break;
	default: // Higher-order interrupt numbers are chip-specific.
		break;
	}

	__asm volatile
	(
	"  pop {lr}\n" // Restore the link reg--ALERT: we may need to modify the LR before performing an exception return.
	"  bx %0\n"
	:                          /* output */
	:"r"(guestExceptionVector) /* input */
	:                          /* clobbered register */
	);
}
#endif

void genericHandler() __attribute__((naked));
void genericHandler() {

	int exceptionNum = getIPSR(); // Read IPSR to get exception number
	void *guestExceptionVector = locateGuestISR(exceptionNum); // Get the address of the guest's exception handler corresponding to the exception number
	uint16_t *offendingInstruction; // Address of instruction that caused the exception

	// Save regs & preserve MSP
	__asm volatile
	(
	"  mrs r0,psp\n"
	"  ldr  %0,[r0,#24]"
	:                   /* output */
	:"r"(offendingInstruction) /* input */
	:"r0"                   /* clobbered register */
	);

	switch(exceptionNum){
		case 2: // NMI
			break;
		case 3: // HardFault
			break;
		case 4: // MemManage
			break;
		case 5: // BusFault
			break;
		case 6: // UsageFault
			break;
		case 7:	 // Reserved
		case 8:
		case 9:
		case 10:
			break;
		case 11: // SVCall
			break;
		case 12: // Reserved for debug
		case 13: // Reserved
			break;
		case 14: // PendSV
			break;
		case 15: // SysTick
			break;
		default: // Higher-order interrupt numbers are chip-specific.
			break;
	}

	__asm volatile
	(
	"  ldr r0,=0\n"
	"  add sp,-32\n"      // Build an exception frame on the stack
	"  str r0,[sp,#28]\n" // Put dummy xPSR on exception frame
	"  str %0,[sp,#24]\n" // Put the address of the guest exception vector on the stack frame--this is the return PC
	"  str lr,[sp,#20]\n" // Put the LR on the guest exception frame. XXX This needs to be changed--modify LR to always return to thread mode.
	"  str r0,[sp,#16]\n" // Zero-out the saved registers
	"  str r0,[sp,#12]\n"
	"  str r0,[sp,#8]\n"
	"  str r0,[sp,#4]\n"
	"  str r0,[sp,#0]\n"
	"  ldr lr, =0xfffffffd\n"  // Overwrite LR, forcing exception return to thread mode
	"  bx lr\n"
	:                          /* output */
	:"r"(guestExceptionVector) /* input */
	:                          /* clobbered register */
	);
}


extern unsigned long _estack;

void *hvVectorTable[] __attribute__ ((aligned (128))) = {
	
	/* Configure Initial Stack Pointer, using linker-generated symbols */
	(void*) (void*) (&_estack),    /* Initial stack pointer */ // HACK ALERT!!!! This is used to bypass attempt to load the initial stack pointer from the OS's vector table in prvPortStartFirstTask, found in port.c. Hack can be avoided by virtualizing the MSR instruction

	(void*) genericHandler,    /* Reset Handler */
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


