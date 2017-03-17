
#include "hermes.h"
#include "instdecode.h"
#include <stdint.h>
#include <string.h>

char hvStack[HV_STACK_SIZE];

int **guestExceptionTable;

char privexe[64]; // memory to hold code to execute privileged instructions.

struct vm *guestList, *currGuest;
struct vm guests[1];

/*
 * dummyfunc
 *
 * This function is used by executePrivilegedInstruction to restore the guest's
 * context and execute the privileged instruction. This function is first copied
 * into RAM, and the NOP instructions are replaced with the privileged
 * instruction to be executed.
 *
 */
void dummyfunc() __attribute__((naked));
void dummyfunc()
{
	__asm volatile (
	"  push {r1-r12,r14}\n"
	"  ldr r1,=guest_regs\n"
	"  ldm r1,{r0-r12,r14}\n"
	"  nop\n"
	"  nop\n"
	"  push {r0}\n"
	"  ldr r0,=guest_regs+4\n" // Use R0 to point to the guest_regs array
	"  stm r0,{r1-r12,r14}\n"  // Store guest registers
	"  sub r0,r0,4\n"
	"  pop {r1}\n"
	"  str r1, [r0]\n"
	"  pop {r1-r12,r14}\n"
	"  bx lr\n"
	);
}

void hvInit(void *gET) {
	uint32_t oldMSP = 0;
	uint32_t returnAddress;
	
	// Save regs & preserve MSP
	__asm volatile
	(
	"  push {r1-r12,r14}\n"
	"  mrs %0,msp\n"
	"  msr psp,%0\n"
	"  add %0,%0,52\n"
	"  msr msp,%0\n"
	:                   /* output */
	:"r"(oldMSP) /* input */
	:                   /* clobbered register */
	);
	uint32_t stackTop = (uint32_t)hvStack+HV_STACK_SIZE-4*14;

	// Save return address
	__asm volatile
	(
	"  mov %0,r14\n"
	:"=r"(returnAddress) /* output */
	:                    /* input */
	:                    /* clobbered register */
	);
	
	// Copy register contents from old stack to new stack
	
	// Set up a linked list of guests
	guestList = &guests[0]; // For now, we have only one guest
	currGuest = &guests[0];
	
	currGuest->status = STATUS_PROCESSOR_MODE_MASTER; // Start the guest in Master (handler) mode.
	
	// Set up guest data structures.
	guestExceptionTable = gET;
	currGuest->vectorTable = gET;
	
	SHCSR = (1<<18) | (1<<17) | (1<<16); // Enable bus, usage, and memmanage faults

	//CCR &= ~((1<<16) | (1<<17)); // Disable instruction and data caches

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

	// return
	__asm volatile
	(
	"  pop {r1-r12,r14}\n"
	"  bx lr\n"
	);
}

/*
 * getIPSR()
 *
 * Reads the value of the IPSR, which indicates what interrupt number caused
 * the current exception..
 */
uint32_t getIPSR() ;
uint32_t getIPSR() {
	uint32_t ipsrVal = 0;
	__asm("  mrs %0, IPSR\n"
	      	:"=r"(ipsrVal) // output
	      	:              // input
	      	:              // clobbered register
	      	);
	return ipsrVal;
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

uint32_t guest_regs[15];

/*
 * genericHandler
 *
 * ASM-only handler that stores user regs and calls the C exceptionProcessor,
 * which does the real work. Because of quirks in the GNU C compiler, the
 * assembly code used to store the registers could not be integrated in with
 * the exceptionProcessor handler.
 *
 */
void genericHandler() __attribute__((naked));
void genericHandler(){
	// Save the guest registers into the guest registers array
	__asm volatile (
	"  push {lr}\n"              // Preserve the LR, which stores the EXC_RETURN value.
	"  ldr r0,=guest_regs+4\n"   // Use R0 to point to the guest_regs array
	"  stm r0,{r1-r12}\n"        // Store guest registers
	"  sub r0,r0,4\n"
	"  mrs r1,psp\n"             // Point R1 to the PSP (exception stack frame)
	"  ldr r2,[r1]\n"            // Retrieve guest's R0 from the exception stack frame (store it in R2)
	"  str r2, [r0]\n"           // Put guest's R0 into guest_regs
	"  ldr r2,[r1,#20]\n"        // Get the guest's LR off the exception stack frame
	"  str r2,[r0,#56]\n"        // Put the guest's LR into guest_regs
	"  ldr r0,=currGuest\n"      // Get a pointer to currGuest in r0
	"  ldr r0,[r0]\n"            // Point R0 to the currently executing guest
	"  str r14,[r0,#24]\n"       // Store EXC_RETURN in the currGuest struct
	"  bl exceptionProcessor\n"
	"  ldr r4,=guest_regs\n"     // restore registers and return
	"  mrs r5,psp\n"             // Point R2 to the PSP (exception stack frame)
	"  ldm r4,{r0-r3}\n"         // Get r0-r3 from guest_regs
	"  ldr r12,[r4,#48]\n"       // Get R12 from guest_regs
	"  ldr r14,[r4,#56]\n"       // Get R14 (LR) from guest_regs
	"  stm r5,{r0-r3,r12,r14}\n" // Put guest reigsters from guest_regs array onto the exception stack frame
	"  ldm r4,{r0-r12}\n"
	"  pop {lr}\n"               // Pop the LR, which olds EXC_RETURN, saved on the stack at beginning of genericHandler
	"  bx lr\n"
	);
}

/*
 * executePrivilegedInstruction
 *
 * Executes a privileged instruction, pointed to by offendingInstruction.
 *
 *
 */
void executePrivilegedInstruction(uint16_t *offendingInstruction, struct inst *instruction)
{
	// TODO: put the instruction to execute inside dummyfunc, replacing NOPs
	char *srcptr = (char*)&dummyfunc;
	memcpy(privexe, (char*)((uint32_t)srcptr & 0xfffffffe), 64); // Copy dummyfunc into privexe array. NOTE: the (srcptr & 0xfe) is a workaround because gcc always adds 1 to the address of a function pointer because
	memcpy(privexe+10, (char*)offendingInstruction, instruction->nbytes);

	__asm volatile(
	"  push {lr}\n"
	"  isb sy\n"
	"  dsb sy\n"
	"  ldr r0,=privexe\n"
	"  orr r0,r0,1\n" // Set the LSB to enable thumb execution mode
	"  blx r0\n"
	"  pop {lr}\n"
	:         /* output */
	:         /* input */
	:  "r0"   /* clobbered register */);
}

/*
 * trackImpreciseBusFault
 *
 * Starting at offendingInstruction, traces back one instruction at a time,
 * searching for instructions that could have caused a bus fault. This routine
 * assumes that the instruction that caused the imprecise bus fault is a load/
 * store, which may not be the case.
 *
 * NOTE: there are ways to defeat this, which could cause the hypervisor to
 * hang. For instance, you could do an load or store to a privileged memory
 * region pointed to by Rx followed immediately by a load to Rx. If the first
 * load/store causes an imprecise exception, we won't be able to determine that
 * it was the offending instruction because by the time the exception handler
 * gets called, the register that held the effective address will already have
 * been overwritten. There are probably ways around this, like tracing back
 * further to see when Rx was last loaded, and then trying to figure out what
 * address it held, but that seems like a bitch.
 *
 */
uint16_t *trackImpreciseBusFault(uint16_t *offendingInstruction, uint32_t busFaultAddress)
{
	int i;
	struct inst instruction;
	uint32_t effective_address;
	
	// Trace back 5 (two-byte) instructions
	for(i = 0; i < 5 ; i++){
		instDecode(&instruction, offendingInstruction-i);
		
		effective_address = -1;
		
		// Compute the effective address of the instruction
		switch(instruction.type){
		case THUMB_TYPE_LDSTREG:
			effective_address = guest_regs[instruction.Rn] + guest_regs[instruction.Rm];
			break;
		case THUMB_TYPE_LDSTWORDBYTE:
		case THUMB_TYPE_LDSTHALFWORD:
		case THUMB_TYPE_LDSTSINGLE:
			effective_address = guest_regs[instruction.Rn] + instruction.imm;
			break;
		default:
			break;
		}
		if(effective_address == busFaultAddress){
			return offendingInstruction - i;
		}
	}
	
	return (uint16_t*)-1;
}


uint16_t *trackMRS(uint16_t *offendingInstruction){
	int i;
	struct inst instruction;

	for(i = 0; i < 5 ; i++){
		instDecode(&instruction, offendingInstruction-i);
		if((instruction.type == THUMB_TYPE_MRS) || (instruction.type == THUMB_TYPE_MSR)) {
			return offendingInstruction-i;
		}
	}
	return (uint16_t*)-1;
}

/*
 * exceptionProcessor
 *
 * This is the hypervisor's exception handling routine. It calls the guest's
 * ISR if necessary, or executes a privileged instruction if necessary.
 *
 */
void exceptionProcessor() {

	int exceptionNum = getIPSR(); // Read IPSR to get exception number
	void *guestExceptionVector = locateGuestISR(exceptionNum); // Get the address of the guest's exception handler corresponding to the exception number
	uint16_t *offendingInstruction, *guestPC; // Address of instruction that caused the exception
	uint32_t *psp;
	uint32_t busFaultStatus,auxBusFaultStatus,usageFaultStatus;
	uint32_t busFaultAddress;
	uint32_t *newStackFrame;
	struct inst instruction;


	// Get the address of the instruction that caused the exception and the PSP
	__asm
	(
	"  mrs %1,psp\n"
	"  ldr  %0,[%1,#24]\n"
	"  ldr  %2,[%1,#28]\n"
	:"=r"(guestPC), "=r" (psp), "=r"(currGuest->PSR)  /* output */
	:                           /* input */
	:                           /* clobbered register */
	);

	switch(exceptionNum){
		case 2: // NMI
			break;
		case 3: // HardFault

			return;
		case 4: // MemManage
		
			currGuest->MSP = psp + 32; // Store the guest's MSP

			__asm volatile(
			"msr psp,%0\n"
			:                            /* output */
			:"r"(currGuest->PSP)         /* input */
			:                            /* clobbered register */
			);

			break;
		case 5: // BusFault
			busFaultStatus = BFSR;
			auxBusFaultStatus = ABFSR;
			busFaultAddress = BFAR;
			
			if(((busFaultStatus & BFSR_IMPRECISEERR_MASK) != 0) /*&& ((busFaultStatus & BFSR_BFARVALID_MASK) != 0)*/){ // Imprecise bus fault?
				offendingInstruction = trackImpreciseBusFault(guestPC, busFaultAddress);
			} else {
				offendingInstruction = guestPC;
				
				// On a precise exception, we will increment the guest's PC past the offending instruction and move on.
				
				// Decode the instruction that caused the problem
				instDecode(&instruction, offendingInstruction);

				// Increment the return address on the exception stack frame
				*(psp+6) += instruction.nbytes;
			}
			if((int32_t)offendingInstruction != -1) { // HACK ALERT!! Hack to avoid problem of unexplained access to 0xe000ed20
 				executePrivilegedInstruction(offendingInstruction, &instruction);
			}
			
			// Clear the BFSR
			BFSR = 0xff;
			break;
		case 6: // UsageFault
			usageFaultStatus = UFSR;
			
			if(usageFaultStatus & 1){ // Check for illegal instruction
				
				// Point the guest's PC past the illegal instruction and continue executing.
				__asm
				(
				"  mrs %1,psp\n"
				"  ldr  %0,[%1,#24]\n"
				"  add  %0,%0,#2\n"
				"  str  %0,[%1,#24]\n"
				:"=r"(guestPC), "=r" (psp)  /* output */
				:                           /* input */
				:                           /* clobbered register */
				);

				offendingInstruction = trackMRS(guestPC-1);
				instDecode(&instruction, offendingInstruction); // Decode the instruction
				switch(instruction.imm){ // instDecode places the register number in instruction.imm
				case MRS_REGISTER_MSP:
					if(instruction.type == THUMB_TYPE_MRS){
						guest_regs[instruction.Rd] = (uint32_t)currGuest->MSP;
					} else if(instruction.type == THUMB_TYPE_MSR){
						char temp_stack_frame[32];
						
						// NOTE: IF THE GUEST THINKS IT'S IN MASTER MODE, SET THE PROCESSOR'S PSP TO POINT TO currGuest->MSP
						if(GUEST_IN_MASTER_MODE(currGuest)){
							currGuest->MSP = (uint32_t*)guest_regs[instruction.Rn]; // Set the guest's MSP
							
							currGuest->MSP -= 32; // create an interrupt stack frame on the guest's MSP
							
							memcpy(temp_stack_frame,psp,32); // Copy old frame to temp buffer in case the stack frames overlap
							memcpy(currGuest->MSP,temp_stack_frame,32); // Copy the interrupt stack frame from the hardware PSP to the guest's MSP.
														
							// Now put the requested new MSP into the hardware PSP, which is the actual stack pointer used by the guest.
							asm("msr psp,%0"
							:                     /* output */
							: "r"(currGuest->MSP) /* input */
							:                     /* clobbered register */
							);
						}
					}
					break;
				case MRS_REGISTER_PSP:
					if((instruction.type == THUMB_TYPE_MRS) && GUEST_IN_MASTER_MODE(currGuest)){
						guest_regs[instruction.Rd] = (uint32_t)currGuest->PSP;
					} else if((instruction.type == THUMB_TYPE_MSR) && GUEST_IN_MASTER_MODE(currGuest)){
						currGuest->PSP = (uint32_t*)guest_regs[instruction.Rn];
					}
					break;
				case MRS_REGISTER_BASEPRI:
					break;
				} // switch(instruction.imm)
				
			}
			UFSR = 0xff; // Clear UFSR
			break;
		case 7:	 // Reserved
		case 8:
		case 9:
		case 10:
			break;
		case 11: // SVCall
			
			newStackFrame = psp - 8;
			
			memcpy(newStackFrame,psp,32);
			*(newStackFrame+6) = (uint32_t)guestExceptionVector;
			
			__asm volatile(
			"msr psp,%0\n"
			:                            /* output */
			:"r"(newStackFrame)         /* input */
			:                            /* clobbered register */
			);
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

	return;

}



/*

#if 0
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
	:                          // output
	:"r"(guestExceptionVector) // input
	:                          // clobbered register
	);
#endif
*/

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


