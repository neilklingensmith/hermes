
/*

MIT License

Copyright (c) 2017 Neil Klingensmith

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
#ifdef HERMES_ETHERNET_BRIDGE
#include "libboard/include/gmacb_phy.h"
#endif
#ifdef HERMES_ETHERNET_BRIDGE
#include "virt_eth.h"
#endif
#include "libchip/chip.h"
#include "nalloc.h"
#include "instdecode.h"
#include <stdint.h>
#include <string.h>

char hvStack[HV_STACK_SIZE];
char privexe[64]; // memory to hold code to execute privileged instructions.
struct vm *guestList = NULL, *currGuest = NULL;
uint32_t guest_regs[15];


int putChar(uint32_t c); // for esp_printf


#ifdef HERMES_ETHERNET_BRIDGE
char eth_buf[ETH_BUF_SIZE+2];
#endif

#ifdef HERMES_ETHERNET_BRIDGE
/* The GMAC driver instance */
sGmacd gGmacd __attribute__ ((aligned (32)));
/* The GMACB driver instance */
GMacb gGmacb __attribute__ ((aligned (32)));

#endif

struct vm *dummyGuest; // LATENCY TESTING ONLY!!!


void *ramVectors[80]  __attribute__ ((aligned (128)));

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
	"  ldr r1,=currGuest\n"      // Use R0 to point to the guest_regs array
	"  ldr r1,[r1]\n"
	"  ldr r1,[r1,#32]\n"
	"  ldm r1,{r0-r12,r14}\n"
	"  nop\n"
	"  nop\n"
	"  push {r0}\n"

	"  ldr r0,=currGuest\n"      // Use R0 to point to the guest_regs array
	"  ldr r0,[r0]\n"
	"  ldr r0,[r0,#32]\n"
	"  add r0,r0,#4\n"
	"  stm r0,{r1-r12,r14}\n"  // Store guest registers
	"  sub r0,r0,4\n"
	"  pop {r1}\n"
	"  str r1, [r0]\n"
	"  pop {r1-r12,r14}\n"
	"  bx lr\n"
	);
}

/*
 * listAdd
 *
 * Adds newElement to a linked list pointed to by list. When calling this function, pass the address of list head.
 *
 */
int listAdd(struct listElement **head, struct listElement *newElement){
	struct listElement *iterator = (struct listElement*)head ;

	// Link element b into the list between iterator and iterator->next.
	newElement->next = iterator->next ;
	newElement->prev = iterator ;

	iterator->next = newElement ;
	
	if(newElement->next != NULL){
		newElement->next->prev = newElement ;
	}
	return 0;
}

int createGuest(void *guestExceptionTable){
	struct vm *newGuest = nalloc(sizeof(struct vm));
	struct scb *newSCB = nalloc(sizeof(struct scb));
	uint32_t *new_guest_regs = nalloc(16*sizeof(uint32_t));
	
	if((uint32_t)newGuest == -1 || (uint32_t)newSCB == -1 || (uint32_t)new_guest_regs == -1){
		return -1;
	}

	listAdd((struct listElement **)&guestList, (struct listElement *)newGuest);
	newGuest->guest_regs = new_guest_regs;
	
	newGuest->virtualSCB = newSCB;
	
	// Initialize the SCB
	memset(newSCB, 0, sizeof(struct scb));
	newGuest->virtualSCB->CPUID = 0x410FC270; // Set CPUID to ARM Cortex M7
	newGuest->virtualSCB->AIRCR = 0xFA050000; // Set AIRCR to reset value
	newGuest->virtualSCB->CCR = 0x00000200;   // Set CCR to reset value
	newGuest->virtualSCB->VTOR = guestExceptionTable;

	newGuest->status = STATUS_PROCESSOR_MODE_MASTER; // Start the guest in Master (handler) mode.

	// Set up the new guest's master stack
	newGuest->MSP = (((uint32_t*)(newGuest->virtualSCB->VTOR))[0] - 32) & 0xfffffff8;
	memset(newGuest->MSP, 0, 32);// zero out the guest's exception stack frame
	((uint32_t*)newGuest->MSP)[6] = ((uint32_t*)newGuest->virtualSCB->VTOR)[1] | 1;
	((uint32_t*)newGuest->MSP)[7] =  (1<<24); // We're returning to an exception handler, so stack frame holds EPSR. Set Thumb bit to 1.

	currGuest = guestList; // Point currGuest to first element in guestList
}

void hvInit() {
	// Initialize the memory allocator
	memInit();
		
	// Copy dummyfunc into RAM. This is used to execute privileged instructions
	char *srcptr = (char*)&dummyfunc;
	memcpy(privexe, (char*)((uint32_t)srcptr & 0xfffffffe), 64); // Copy dummyfunc into privexe array. NOTE: the (srcptr & 0xfe) is a workaround because gcc always adds 1 to the address of a function pointer because

	// Set up a linked list of guests
	guestList = NULL;
	currGuest = NULL;
	
	CORTEXM7_SHCSR = (1<<18) | (1<<17) | (1<<16); // Enable bus, usage, and memmanage faults

	//CORTEXM7_CCR &= ~((1<<16) | (1<<17)); // Disable instruction and data caches

	//SCB_SHPR1 = 0x00ffffff; // Set UsageFault, BusFault, MemManage exceptions to highest priority
}

/*
 * locateGuestISR(int interruptNum)
 *
 * Parses the guest's exception table, and returns the address of the guest's
 * exception handler associated with interruptNum, the exception number.
 */
uint32_t *locateGuestISR(struct vm *guest, int interruptNum){
	return (uint32_t*)((uint32_t*)guest->virtualSCB->VTOR)[interruptNum]; // stub that returns function pointer to guest exception table.
}

/*
 * genericHandler
 *
 * ASM-only handler that stores user regs and calls the C exceptionProcessor,
 * which does the real work. Because of quirks in the GNU C compiler, the
 * assembly code used to store the registers could not be integrated in with
 * the exceptionProcessor handler.
 *
 */
void genericHandler(){
	// Save the guest registers into the guest registers array
	
	// TODO: Modify this assembly code to use inline gcc syntax so we don't have to hardcode struct member offsets
	__asm volatile (
	"  cpsid i\n"
	"  push {lr}\n"              // Preserve the LR, which stores the EXC_RETURN value.
	"  ldr r0,=currGuest\n"      // Use R0 to point to the guest_regs array
	"  ldr r0,[r0]\n"
	"  ldr r0,[r0,#32]\n"
	"  add r0,r0,#4\n"
	
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
	);
	
	exceptionProcessor();
	
	__asm volatile (
	"  ldr r4,=currGuest\n"      // R4 <- currGuest->guest_regs
	"  ldr r4,[r4]\n"
	"  ldr r4,[r4,#32]\n"
	
	"  mrs r5,psp\n"             // Point R5 to the PSP (exception stack frame)
	"  ldm r4,{r0-r3}\n"         // Get r0-r3 from guest_regs
	"  ldr r12,[r4,#48]\n"       // Get R12 from guest_regs
	"  ldr r14,[r4,#56]\n"       // Get R14 (LR) from guest_regs
	"  stm r5,{r0-r3,r12,r14}\n" // Put guest reigsters from guest_regs array onto the exception stack frame
	"  ldm r4,{r0-r12}\n"
	"  pop {lr}\n"               // Pop the LR, which holds EXC_RETURN, saved on the stack at beginning of genericHandler
	"  cpsie i\n"
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
	char *srcptr = (char*)&dummyfunc;
	memcpy(privexe, (char*)((uint32_t)srcptr & 0xfffffffe), 64); // Copy dummyfunc into privexe array. NOTE: the (srcptr & 0xfe) is a workaround because gcc always adds 1 to the address of a function pointer because
	*((uint32_t*)(privexe+14)) = 0xbf00bf00; // Set the target instructions in privexe to NOPs
	
	memcpy(privexe+14, (char*)offendingInstruction, instruction->nbytes);
	SCB_InvalidateICache();
	__asm volatile(
	"  push {lr}\n"
	"  dsb\n"
	"  isb\n"
	"  dsb\n"
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
		
		effective_address = (uint32_t) effectiveAddress(&instruction, currGuest);

		if(effective_address == busFaultAddress){
			return offendingInstruction - i;
		}
	}
	
	return (uint16_t*)-1;
}

/*
 * findLastPrivilegedMemoryAccess
 *
 * Beginning at startingAddress, traces backwards through program memory to
 * find the last load/store instruction that accesses a privilged memory
 * address. Returns the address of the instruction if it finds one, -1
 * otherwise.
 *
 * This function is used as a backup in case trackImpreciseBusFault cannot
 * locate the instruction that caused a bus fault.
 *
 */
uint16_t *findLastPrivilegedMemoryAccess(uint16_t *startingAddress)
{
	int i;
	struct inst instruction;
	uint32_t effective_address;

	for(i = 0; i < 10; i++){
		instDecode(&instruction, startingAddress-i);
		
		effective_address = (uint32_t) effectiveAddress(&instruction, currGuest);

		if((effective_address >= 0xe0000000) && (effective_address <= 0xE00FFFFF)){
			return startingAddress-i;
		}
	}
	return (uint16_t*)-1;
}

/*
 * trackMRS
 *
 * Starting at the address offendingInstruction, trace back through the program
 * trace sequentially in memory looking for MRS or MSR instructions. Return the
 * address of the MRS/MSR instruction.
 *
 */
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
 * emulateSCBAccess
 *
 * Emulate accesses to the SCB made by guest with instruction at location.
 *
 */
int emulateSCBAccess(struct vm *guest, uint16_t *location, struct inst *instruction){

	uint32_t *ea = effectiveAddress(instruction, guest);
	uint32_t icsrVal;

	if(instruction->mnemonic[0] == 'L'){
		// Load from SCB
		switch((uint32_t)ea){
		case (uint32_t)&CORTEXM7_ICSR: // Loads from the ICSR.
			// Different load instruction types use different register fields to specify
			// the destination of the load, so check which one we're using and put the
			// virtual ICSR value into that reg.
			if(instruction->Rd != 0xff){
				guest->guest_regs[instruction->Rd] = guest->virtualSCB->ICSR;
			} else if (instruction->Rt != 0xff) {
				guest->guest_regs[instruction->Rt] = guest->virtualSCB->ICSR;
			}
			return 0;
		case (uint32_t)&CORTEXM7_VTOR: // Loads from the VTOR
			if(instruction->Rd != 0xff){
				guest->guest_regs[instruction->Rd] = guest->virtualSCB->VTOR;
			} else if (instruction->Rt != 0xff) {
				guest->guest_regs[instruction->Rt] = guest->virtualSCB->VTOR;
			}
			return 0;
		case (uint32_t)&CORTEXM7_SHCSR: // Loads from SHCSR
			if(instruction->Rd != 0xff){
				guest->guest_regs[instruction->Rd] = guest->virtualSCB->SHCSR;
			} else if (instruction->Rt != 0xff) {
				guest->guest_regs[instruction->Rt] = guest->virtualSCB->SHCSR;
			}
		default:
			// Default action is to blindly execute privileged SCB access
			break;
		}
	} else if (instruction->mnemonic[0] == 'S') {
		// Store to SCB
	switch((uint32_t)ea){
		case (uint32_t)&CORTEXM7_ICSR: // Stores to the ICSR
		// Different load instruction types use different register fields to specify
		// the destination of the load, so check which one we're using and put the
		// virtual ICSR value into that reg.
		if(instruction->Rd != 0xff){
			icsrVal = guest->guest_regs[instruction->Rd];
		} else if (instruction->Rt != 0xff) {
			icsrVal = guest->guest_regs[instruction->Rt];
		}
		if(icsrVal & (1<<28)){ // PendSV Set Bit
			guest->virtualSCB->ICSR |= (1<<28);
		}
		
		if(icsrVal & (1<<27)){ // PendSV Clear bit
			guest->virtualSCB->ICSR &= ~(1<<28);
		}
		return 0;
		case (uint32_t)&CORTEXM7_VTOR: // VTOR
			// Different load instruction types use different register fields to specify
			// the destination of the load, so check which one we're using and put the
			// virtual ICSR value into that reg.
			if(instruction->Rd != 0xff){
				guest->virtualSCB->VTOR = guest->guest_regs[instruction->Rd];
			} else if (instruction->Rt != 0xff) {
				guest->virtualSCB->VTOR = guest->guest_regs[instruction->Rt];
			}
			return 0;
		case (uint32_t)&CORTEXM7_SHCSR: // SHCSR
			if(instruction->Rd != 0xff){
				guest->virtualSCB->SHCSR = guest->guest_regs[instruction->Rd];
				} else if (instruction->Rt != 0xff) {
				guest->virtualSCB->SHCSR = guest->guest_regs[instruction->Rt];
			}
			return 0;
		default:
		// Default action is to blindly execute privileged SCB access
		break;
	}
	}
	
	executePrivilegedInstruction(location, instruction);
	return 0;
}

int hvScheduler(uint32_t *psp){
	// Save the current guest's context.
	if(GUEST_IN_MASTER_MODE(currGuest)){
		currGuest->MSP = psp;
	} else {
		currGuest->PSP = psp;
	}
	
	// Dumb scheduler. Cycle thru guests.
	if(currGuest->next != NULL){
		currGuest = currGuest->next;
	} else {
		currGuest = guestList;
	}

	// Install the new guest's PSP
	if(GUEST_IN_MASTER_MODE(currGuest)){
		__asm volatile(
		"msr psp,%0\n"
		:                            /* output */
		:"r"(currGuest->MSP)         /* input */
		:                            /* clobbered register */
		);
	} else {
		__asm volatile(
		"msr psp,%0\n"
		:                            /* output */
		:"r"(currGuest->PSP)         /* input */
		:                            /* clobbered register */
		);
	}
	asm("isb");
	asm("dsb");

	return 0;
}

/*
 * exceptionProcessor
 *
 * This is the hypervisor's exception handling routine. It calls the guest's
 * ISR if necessary, or executes a privileged instruction if necessary.
 *
 */
void exceptionProcessor() {

	uint32_t exceptionNum;
	void *guestExceptionVector; // Get the address of the guest's exception handler corresponding to the exception number
	uint16_t *offendingInstruction, *guestPC; // Address of instruction that caused the exception
	uint32_t *psp;
	uint32_t busFaultStatus,auxBusFaultStatus,usageFaultStatus; // Fault status regs.
	uint32_t busFaultAddress; // Address in program mem that caused bus fault
	uint32_t *newStackFrame; // Pointer to new stack frame when we're changing processor state (master/thread) or entering a guest ISR
	struct inst instruction; // Instruction that caused the exception for bus faults and usage faults (emulated privileged instrs)
	uint32_t newLR; // New Link Reg, used when we're setting up to enter a guest exception handler
	uint32_t pkt_len = 0 ;
	static int count = 0;

	count++;
	// Get the address of the instruction that caused the exception and the PSP
	__asm
	(
	"  mrs %1,psp\n"
	"  ldr  %0,[%1,#24]\n"
	"  ldr  %2,[%1,#28]\n"
	"  mrs %3,IPSR\n"
	:"=r"(guestPC), "=r" (psp), "=r"(currGuest->PSR), "=r"(exceptionNum)  /* output */
	:                           /* input */
	:                           /* clobbered register */
	);

	// Check if we got to hard fault because of escalation of a bus fault or a usage fault
	// This can be caused by a usage fault or hard fault when PRIMASK = 1
	if(exceptionNum == ARM_CORTEX_M7_HARDFAULT_ISR_NUM){
		usageFaultStatus = CORTEXM7_UFSR;
		busFaultStatus = CORTEXM7_BFSR;
		if(usageFaultStatus != 0){
			exceptionNum = ARM_CORTEX_M7_USAGEFAULT_ISR_NUM;
//		} else if ((busFaultStatus & 0x39) != 0){
		} else if (busFaultStatus != 0){
			exceptionNum = ARM_CORTEX_M7_BUSFAULT_ISR_NUM;
		} else if ((CORTEXM7_MMFSR & 0x7f) != 0){
			exceptionNum = ARM_CORTEX_M7_MEMMANAGEFAULT_ISR_NUM;
		}
	}

	switch(exceptionNum){
		case 2: // NMI
			break;
		case 3: // HardFault
			return;
		case 4: // MemManage: Caused by bx lr instruction or similar trying to put EXEC_RETURN value into the PC

			currGuest->MSP = ((*(psp+7)>>9 & 1)*4)+psp + 8; // Store the guest's MSP

//			if((*(psp+6) & 0xe) != 0) {
//			if((currGuest->guest_regs[14] & 0xe) != 0){ // Handles exception returns via BX LR instrcution, assuming EXC_RETURN is in the LR
			if(((uint32_t)guestPC & 0xe) != 0){ // Handles popping the PC off the stack and BX LR

				// If EXEC_RETURN word has 0x9 or 0xD in the low order byte, then we're returning to thread mode
				SET_PROCESSOR_MODE_THREAD(currGuest);
			
				__asm volatile(
				"msr psp,%0\n"
				:                            /* output */
				:"r"(currGuest->PSP)         /* input */
				:                            /* clobbered register */
				);

				// Preserve registers that were clobbered by the exception handler
				newStackFrame = currGuest->PSP;
				currGuest->guest_regs[0] = newStackFrame[0];
				currGuest->guest_regs[1] = newStackFrame[1];
				currGuest->guest_regs[2] = newStackFrame[2];
				currGuest->guest_regs[3] = newStackFrame[3];
				currGuest->guest_regs[12] = newStackFrame[4];
				currGuest->guest_regs[14] = newStackFrame[5];

			} else {
				// Otherwise we're returning to master mode
				SET_PROCESSOR_MODE_MASTER(currGuest);
				
				__asm volatile(
				"msr psp,%0\n"
				"msr basepri,%1\n"
				:                                             /* output */
				:"r"(currGuest->MSP), "r"(currGuest->BASEPRI) /* input */
				:                                             /* clobbered register */
				);
				// Preserve registers that were clobbered by the exception handler
				newStackFrame = currGuest->MSP;
				currGuest->guest_regs[0] = newStackFrame[0];
				currGuest->guest_regs[1] = newStackFrame[1];
				currGuest->guest_regs[2] = newStackFrame[2];
				currGuest->guest_regs[3] = newStackFrame[3];
				currGuest->guest_regs[12] = newStackFrame[4];
				currGuest->guest_regs[14] = newStackFrame[5];
			}
			CORTEXM7_MMFSR = 0xff;
			break;
		case 5: // BusFault
			busFaultStatus = CORTEXM7_BFSR;
			//auxBusFaultStatus = CORTEXM7_ABFSR;
			busFaultAddress = CORTEXM7_BFAR;
			
			if((busFaultStatus & BFSR_IMPRECISEERR_MASK) != 0){ // Imprecise bus fault?
				offendingInstruction = trackImpreciseBusFault(guestPC, busFaultAddress);
				
				// If trackImpreciseBusFault is unable to locate the instuction
				// that caused the bus fault, then use
				// findLastPrivilegedMemoryAccess to search backwards in program
				// memory to locate the last instruction that accessed a
				// privileged memory region. This is the most likely culprit.
				if(offendingInstruction == (uint16_t*)-1){
					offendingInstruction = findLastPrivilegedMemoryAccess(guestPC);
				}
				if(offendingInstruction != (uint16_t*)-1){
					instDecode(&instruction, offendingInstruction);
				}
			} else {
				offendingInstruction = guestPC;
				// Decode the instruction that caused the problem
				instDecode(&instruction, offendingInstruction);
				// Increment the return address on the exception stack frame
				*(psp+6) += instruction.nbytes;
			}

			if((int32_t)offendingInstruction != (uint16_t*)-1) { // HACK ALERT!! Hack to avoid problem of unexplained access to 0xe000ed20

				uint32_t *ea = (uint32_t*)effectiveAddress(&instruction,currGuest);
				
				// TODO: Should emulate NVIC here too
				
				// If this is a load/store to the SCB, then we have special code to emulate SCB accesses.
				if(((ea >= 0xe000ed00) && (ea < 0xe000ed40) || (ea == 0xe000e008)) &&
				   ((instruction.type == THUMB_TYPE_LDSTREG) ||
				   (instruction.type == THUMB_TYPE_LDSTWORDBYTE) ||
				   (instruction.type == THUMB_TYPE_LDSTHALFWORD) ||
				   (instruction.type == THUMB_TYPE_LDSTSINGLE))){
					emulateSCBAccess(currGuest, offendingInstruction, &instruction);
				} else {
					// If not a load/store to the SCB, then just blindly execute the instr.
 					executePrivilegedInstruction(offendingInstruction, &instruction);
				}
			}
			
			// Clear the BFSR
			CORTEXM7_BFSR = 0xff;
			break;
		case 6: // UsageFault
			usageFaultStatus = CORTEXM7_UFSR;

			if(usageFaultStatus & 1){ // Check for illegal instruction

				// LATENCY TEST ONLY!!!!
				instDecode(&instruction, guestPC); // Decode the instruction
				if(instruction.imm == 1){
					asm("ldr r0,=0xE0001004\n"
					"ldr %0,[r0]\n"
					: "=r"(currGuest->guest_regs[0])
					:
					: "r0");
					*(psp+6) += 2;
					CORTEXM7_UFSR = 0xff;
					break;
				}
				// end latency testing
					



				// Point the guest's PC past the illegal instruction and continue executing.
				*(psp+6) += 2;

				offendingInstruction = trackMRS(guestPC-1); // Find the MRS/MSR instruction assoc'd with the illegal instruction.
				if(offendingInstruction != -1){
					instDecode(&instruction, offendingInstruction); // Decode the instruction
					switch(instruction.imm){ // instDecode places the register number in instruction.imm
					case MRS_REGISTER_MSP:
						if(instruction.type == THUMB_TYPE_MRS){
							currGuest->guest_regs[instruction.Rd] = (uint32_t)currGuest->MSP;
						} else if(instruction.type == THUMB_TYPE_MSR){
							char temp_stack_frame[32];
						
							// NOTE: IF THE GUEST THINKS IT'S IN MASTER MODE, SET THE PROCESSOR'S PSP TO POINT TO currGuest->MSP
							if(GUEST_IN_MASTER_MODE(currGuest)){
								currGuest->MSP = (uint32_t*)currGuest->guest_regs[instruction.Rn]; // Set the guest's MSP

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
							currGuest->guest_regs[instruction.Rd] = (uint32_t)currGuest->PSP;
						} else if((instruction.type == THUMB_TYPE_MSR) && GUEST_IN_MASTER_MODE(currGuest)){
							currGuest->PSP = (uint32_t*)currGuest->guest_regs[instruction.Rn];
						}
						break;
					case MRS_REGISTER_BASEPRI:
						// Here we're blindly setting the BASEPRI register when requested by the guest. This strategy is probably not good. Probably should be emulated.
						if((instruction.type == THUMB_TYPE_MRS) && GUEST_IN_MASTER_MODE(currGuest)){
							asm("mrs %0,basepri\n"
							:                     /* output */
							: "r"(currGuest->guest_regs[instruction.Rd]) /* input */
							:                     /* clobbered register */
							);
						} else if((instruction.type == THUMB_TYPE_MSR) && GUEST_IN_MASTER_MODE(currGuest)){
							currGuest->BASEPRI = currGuest->guest_regs[instruction.Rn]; // Emulating BASEPRI
							asm("msr basepri,%0\n"
							:                     /* output */
							: "r"(currGuest->guest_regs[instruction.Rn]) /* input */
							:                     /* clobbered register */
							);
						}
						break;
					} // switch(instruction.imm)
				}// if(offendingInstruction != -1)
				///////////////////////////////////////////////////////////////
				// NOTE: Here we have an undefined instruction that does not
				// seem to be related to an MRS or CPS. We should probably
				// invoke the guest's exception vector.
			} else {
				// Search RAM for 0x80000060
				int *ptr =  0x20400000;
				while(ptr < (0x20400000 + 0x60000)){
					if(*ptr == 0x80000060){
						while(1);
					}
					ptr++;
				}
				while(1);
			}
			CORTEXM7_UFSR = 0xff; // Clear UFSR
			break;
		case 7:	 // Reserved
		case 8:
		case 9:
		case 10:
			break;
		case 11: // SVCall
			
			if(GUEST_IN_MASTER_MODE(currGuest)){
				newStackFrame = psp - 8;
			} else {
				newStackFrame = currGuest->MSP - 8;
			}
			
			memcpy(newStackFrame,psp,32);
			*(newStackFrame+6) = (uint32_t)locateGuestISR(currGuest,exceptionNum) | 1; // find the guest's SVCall exception handler
			SET_PROCESSOR_MODE_MASTER(currGuest); // Change to master mode since we're jumping to an exception processor
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
			hvScheduler(psp);
			
			// re-read the PSP since it might have been changed by the hvScheduler
			__asm
			(
			"  mrs %0,psp\n"
			: "=r" (psp)                /* output */
			:                           /* input */
			:                           /* clobbered register */
			);
			
			if(GUEST_IN_MASTER_MODE(currGuest)){
				newStackFrame = psp - 8;
				newLR = 0xFFFFFFF1;
			} else {
				newStackFrame = currGuest->MSP - 8;
				currGuest->PSP = psp;
				newLR = 0xFFFFFFFD;
			}

			// Set up a stack frame so when we return from this exception we will return to the guest's SysTick handler
			memcpy(newStackFrame,currGuest->guest_regs, 16);
			newStackFrame[4] = currGuest->guest_regs[12];
			newStackFrame[5] = newLR; // SHOULD BE currGuest->guest_regs[14]
			*(newStackFrame+6) = (uint32_t)locateGuestISR(currGuest,ARM_CORTEX_M7_SYSTICK_ISR_NUM) | 1; // Find the guest's PendSV handler
			newStackFrame[7] = (1<<24); // We're returning to an exception handler, so stack frame holds EPSR. Set Thumb bit to 1.
			currGuest->guest_regs[14] = newLR; // put newLR into guest_regs, since the stack frame version will be overwritten by generic_handler before returning to the guest. Note: I think the guest's LR should be saved on its stack, so this should be ok.

			SET_PROCESSOR_MODE_MASTER(currGuest); // Change to master mode since we're jumping to an exception processor
			__asm volatile(
			"msr psp,%0\n"
			:                           /* output */
			:"r"(newStackFrame)         /* input */
			:                           /* clobbered register */
			);
			return;
			
#ifdef HERMES_ETHERNET_BRIDGE
		case SAME70_ETHERNET_ISR_NUM:
			//GMACD_Handler_Hermes(&gGmacd,0);
			
			GMACD_Handler(&gGmacd,0);
			
			pkt_len = 0;
			while ( GMACD_OK == GMACD_Poll( &gGmacd, (uint8_t*)eth_buf, ETH_BUF_SIZE, &pkt_len, 0) )
			{
				// We Rx'd a frame. Process it.
				extern struct virt_eth *ifList;
				uint32_t *destMac = eth_buf;
				struct virt_eth *iterator = ifList;

				if((*destMac == -1) && (*(uint32_t*)((uint32_t)destMac+2) == -1)){ // Broadcast frame
					while(iterator != NULL){ // Loop thru all interfaces
						if(iterator->currRxBufWrite->addr.bm.bOwnership == 0){ // Make sure ownership bit is clear so we can write
							memcpy(iterator->currRxBufWrite->addr.bm.addrDW, eth_buf, pkt_len); // Copy frame data to Rx buffer
							iterator->currRxBufWrite->length = pkt_len; // Copy length
							iterator->currRxBufWrite->addr.bm.bOwnership = 1; // Set ownership bit to 1 to indicate that buffer contains new data
						}
						if(iterator->currRxBufWrite->addr.bm.bWrap == 1){ // Point currRxBuf to next element in the descriptor list.
							iterator->currRxBufWrite = iterator->rxBufDescList;
						} else {
							iterator->currRxBufWrite++;
						}
						iterator = iterator->next;
					}
				} else { // Search interfaces for matching dest MAC
					while(iterator != NULL){ // Loop thru all interfaces
						// Compare MAC address of the interface to the packet's dest MAC
						if((*destMac == (*(uint32_t*)((uint32_t)iterator->macAddress))) && (*(uint32_t*)((uint32_t)destMac+2) ==  (*(uint32_t*)((uint32_t)iterator->macAddress+2)))){
							if(iterator->currRxBufWrite->addr.bm.bOwnership == 0){ // Make sure ownership bit si clear so we can write
								memcpy(iterator->currRxBufWrite->addr.bm.addrDW, eth_buf, pkt_len); // Copy frame data to Rx buffer
								iterator->currRxBufWrite->length = pkt_len; // Copy length
								iterator->currRxBufWrite->addr.bm.bOwnership = 1; // Set ownership bit to 1 to indicate that buffer contains new data
							}
							if(iterator->currRxBufWrite->addr.bm.bWrap == 1){ // Point currRxBuf to next element in the descriptor list.
								iterator->currRxBufWrite = iterator->rxBufDescList;
								} else {
								iterator->currRxBufWrite++;
							}
						}
						iterator = iterator->next;
					}
				}
			}
			break;
#endif

		default: // Higher-order interrupt numbers are chip-specific.


			
			// DEBUG ONLY!!
			// LATENCY TESTING!!

			// Save the current guest's context.
			if(GUEST_IN_MASTER_MODE(currGuest)){
				currGuest->MSP = psp;
				} else {
				currGuest->PSP = psp;
			}
			if(exceptionNum == 0x1e){
				currGuest = dummyGuest;
			}
			// Install the new guest's PSP
			if(GUEST_IN_MASTER_MODE(currGuest)){
				__asm volatile(
				"msr psp,%0\n"
				:                            /* output */
				:"r"(currGuest->MSP)         /* input */
				:                            /* clobbered register */
				);
				} else {
				__asm volatile(
				"msr psp,%0\n"
				:                            /* output */
				:"r"(currGuest->PSP)         /* input */
				:                            /* clobbered register */
				);
			}
			// re-read the PSP since it might have been changed by changing the currGuest
			__asm
			(
			"  mrs %0,psp\n"
			: "=r" (psp)                /* output */
			:                           /* input */
			:                           /* clobbered register */
			);
/*
			currGuest->BASEPRI = 0xff;
			asm("mov r0,0xff\n"
			    "msr basepri,r0\n":::"r0");
*/
			////////
			// END DEBUG LATENCY TESTING




			if(GUEST_IN_MASTER_MODE(currGuest)){
				newStackFrame = psp - 8;
				newLR = 0xFFFFFFF1;
			} else {
				newStackFrame = currGuest->MSP - 8;
				currGuest->PSP = psp;
				newLR = 0xFFFFFFFD;
			}

			// Set up a stack frame so when we return from this exception we will return to the guest's SysTick handler
			memcpy(newStackFrame,currGuest->guest_regs, 16);
			newStackFrame[4] = currGuest->guest_regs[12];
			newStackFrame[5] = newLR; // SHOULD BE currGuest->guest_regs[14]
			*(newStackFrame+6) = (uint32_t)locateGuestISR(currGuest,exceptionNum) | 1; // Find the guest's PendSV handler
			newStackFrame[7] = (1<<24); // We're returning to an exception handler, so stack frame holds EPSR. Set Thumb bit to 1.
			currGuest->guest_regs[14] = newLR; // put newLR into guest_regs, since the stack frame version will be overwritten by generic_handler before returning to the guest. Note: I think the guest's LR should be saved on its stack, so this should be ok.

			SET_PROCESSOR_MODE_MASTER(currGuest); // Change to master mode since we're jumping to an exception processor

			__asm volatile(
			"msr psp,%0\n"
			"mrs %1,basepri\n" // Save the guest's BASEPRI
			"movs r0, 0xff\n"  // set the guest's BASEPRI to 0xff to disable interrupts while we're running the guest's ISR
			"msr basepri,r0\n"
			:"=r"(currGuest->BASEPRI)   /* output */
			:"r"(newStackFrame)         /* input */
			:"r0"                       /* clobbered register */
			);
			return;
	}

	// Check to see if we have a PendSV exception pending on this guest.
	// Make sure we are not already executing a PendSV exception handler.
	if((currGuest->virtualSCB->ICSR & (1<<28)) && !GUEST_IN_MASTER_MODE(currGuest)) {
		// re-read the PSP since it might have been changed by the MemManage handler
		__asm
		(
		"  mrs %0,psp\n"
		: "=r" (psp)                /* output */
		:                           /* input */
		:                           /* clobbered register */
		);
		// Clear the PendSVSet bit in ICSR
		currGuest->virtualSCB->ICSR &= ~(1<<28);

		newStackFrame = currGuest->MSP - 8;
		currGuest->PSP = psp;

		// Set up a stack frame so when we return from this exception, we go to the guest's PendSV handler.
		// Copy guest_regs[r0:r3] onto the new stack frame
		memcpy(newStackFrame,currGuest->guest_regs, 16);
		newStackFrame[4] = currGuest->guest_regs[12];
		newStackFrame[5] = currGuest->guest_regs[14];
		newStackFrame[6] = (uint32_t)locateGuestISR(currGuest,ARM_CORTEX_M7_PENDSV_ISR_NUM); // Find the guest's PendSV handler
		newStackFrame[7] = (1<<24); // We're returning to an exception handler, so stack frame holds EPSR. Set Thumb bit to 1.

		currGuest->guest_regs[14] = 0xFFFFFFFD;
		SET_PROCESSOR_MODE_MASTER(currGuest); // Change to master mode since we're jumping to an exception processor
		__asm volatile(
		"msr psp,%0\n"
		:                           /* output */
		:"r"(newStackFrame)         /* input */
		:                           /* clobbered register */
		);
	}
	return;
}


/* Initialize segments. These are defined in the linker script */
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
	extern void *exception_table, *exception_table_g2;
	extern void *dummyVectorTable[];
	register uint32_t *pSrc, *pDest; // Must be register variables because if they are stored on the stack (which is in the .bss section), their values will be obliterated when clearing .bss below

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
	
	//LowLevelInit();
	/* Initialize the C library */
	__libc_init_array();
	/* Disable watchdog */
	//WDT_Disable(WDT);

	hvInit();
	
	dummyGuest = createGuest(&dummyVectorTable); // Init FreeRTOS Blinky demo guest
	createGuest(&exception_table); // Init FreeRTOS Blinky demo guest

	// Set configurable interrupts to low priority
	pDest = 0xe000e400;
	while(pDest < 0xe000e500){
		//*pDest = 0xffffffff;
		*pDest = 0xc0c0c0c0;
		pDest++;
	}
	((uint8_t*)0xe000e400)[14] = 0xff; // set uart interrupt to highest priority
	// Hardware init
#ifdef HERMES_ETHERNET_BRIDGE
	/* Configure systick for 1 ms. */
	//TimeTick_Configure();
	
	// The Atmel ethernet driver uses the SysTick exception to time delays.
	// Since we haven't started the HV yet, we can't use the HV's exception
	// hooks to call Atmel's library SysTick Handler. Instead, we will
	// temporarily put the library SysTick handler into the exception table and
	// allow it to be called directly for purposes of driver initialization. We
	// then replace the hypervisor's hook when we're done with the init.
	void SysTick_Handler(void);
	ramVectors[15] = SysTick_Handler;

	uint8_t macaddr[] = {0x3a, 0x1f, 0x34, 0x08, 0x54, 0x54};
	//gmac_tapdev_setmac_g1((uint8_t *)macaddr);
	gmac_tapdev_init_hermes();
	CORTEXM7_SYST_CVR = 0; // Reset the system timer so we don't get a SysTick exception before we start the HV
	ramVectors[15] = hvVectorTable[15];
#endif

SCB_EnableICache();
//SCB_EnableDCache();

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

