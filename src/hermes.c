
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


// CPU Device definitions from CMSIS
//#if CPUTYPE == same70q21
#include "same70q21.h"
//#endif

//#include "chip.h"
#include "hermes.h"
#if HERMES_ETHERNET_BRIDGE == 1
//#include "libboard/include/gmacb_phy.h"
#include "virt_eth.h"
#include "libchip/chip.h"
#include "libboard/board.h"
#endif
#include "nalloc.h"
#include "instdecode.h"
#include <stdint.h>
#include <string.h>

///////////////////////////////////////////////
// Function Prototypes

#if HERMES_ETHERNET_BRIDGE == 1
void XDMAC_Handler1();
#endif

///////////////////////////////////////////////
// Global Variables


#if HERMES_INTERNAL_DMA == 1
extern sXdmad lcdSpiDma;
#endif

// Memory in RAM to hold code to execute privileged instructions.
// Defined as a uint16_t in order to align it correctly (and supress compiler warning).
uint16_t privexe[32];

extern struct vm *guestList, *sleepingList, *currGuest; // Defined in the scheduler
extern struct interrupt intlist[NUM_PERIPHERAL_INTERRUPTS];

#if HERMES_ETHERNET_BRIDGE == 1

char eth_buf[ETH_BUF_SIZE+2]; // Buffer for Ethernet frames

sGmacd gGmacd __attribute__ ((aligned (32))); // The GMAC driver instance

GMacb gGmacb __attribute__ ((aligned (32))); // The GMACB driver instance

#endif



/*
 * dummyfunc
 *
 * This function is used by executePrivilegedInstruction to restore the guest's
 * context and execute the privileged instruction. This function is first copied
 * into RAM, and the NOP instructions are replaced with the privileged
 * instruction to be executed.
 *
 */
void dummyfunc(void) __attribute__((naked));
void dummyfunc(void)
{
    __asm volatile (
    "  push {r1-r12,r14}\n"
    "  ldr r1,=currGuest\n"      // Use R0 to point to the guest_regs array
    "  ldr r1,[r1]\n"
    "  ldr r1,[r1,#28]\n"
    "  ldm r1,{r0-r12,r14}\n"
    "  nop\n"
    "  nop\n"
    "  push {r0}\n"

    "  ldr r0,=currGuest\n"      // Use R0 to point to the guest_regs array
    "  ldr r0,[r0]\n"
    "  ldr r0,[r0,#28]\n"
    "  add r0,r0,#4\n"
    "  stm r0,{r1-r12,r14}\n"  // Store guest registers
    "  sub r0,r0,4\n"
    "  pop {r1}\n"
    "  str r1, [r0]\n"
    "  pop {r1-r12,r14}\n"
    "  bx lr\n"
    "  .word  currGuest\n"
    );
}

struct vm *createGuest(void *guestExceptionTable){
    struct vm *newGuest = nalloc(sizeof(struct vm));
    struct scb *newSCB = nalloc(sizeof(struct scb));
    struct sysTick *newSysTick = nalloc(sizeof(struct sysTick));
    struct nvic *newNVIC = nalloc(sizeof(struct nvic));
    
    uint32_t *new_guest_regs = nalloc(16*sizeof(uint32_t));
    
    if((uint32_t)newGuest == (uint32_t)-1 || (uint32_t)newSCB == (uint32_t)-1 || (uint32_t)new_guest_regs == (uint32_t)-1){
        return NULL;
    }

    listAdd(&guestList, newGuest);
    newGuest->guest_regs = new_guest_regs;
    
    // Initialize the SCB
    newGuest->virtualSCB = newSCB;
    memset(newSCB, 0, sizeof(struct scb));
    newGuest->virtualSCB->CPUID = 0x410FC270; // Set CPUID to ARM Cortex M7
    newGuest->virtualSCB->AIRCR = 0xFA050000; // Set AIRCR to reset value
    newGuest->virtualSCB->CCR = 0x00000200;   // Set CCR to reset value
    newGuest->virtualSCB->VTOR = (uint32_t)guestExceptionTable;

    // Initialize the SysTick
    newGuest->virtualSysTick = newSysTick;
    memset(newSysTick, 0, sizeof(struct sysTick));
    
    // Initialize the NVIC
    newGuest->virtualNVIC = newNVIC;
    memset(newNVIC, 0, sizeof(struct nvic));

    newGuest->status = STATUS_PROCESSOR_MODE_MASTER; // Start the guest in Master (handler) mode.
    
    newGuest->isrlist = NULL;

    // Set up the new guest's master stack
    newGuest->MSP = (uint32_t*)((((uint32_t*)(newGuest->virtualSCB->VTOR))[0] - 32) & 0xfffffff8);
    memset(newGuest->MSP, 0, 32);// zero out the guest's exception stack frame
    ((uint32_t*)newGuest->MSP)[6] = ((uint32_t*)newGuest->virtualSCB->VTOR)[1] | 1;
    ((uint32_t*)newGuest->MSP)[7] =  (1<<24); // We're returning to an exception handler, so stack frame holds EPSR. Set Thumb bit to 1.

    currGuest = guestList; // Point currGuest to first element in guestList
    
    return newGuest;
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
static uint32_t *locateGuestISR(struct vm *guest, int interruptNum){
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
    // NOTE: Tried to do this, but GCC keeps using R3 as a scratch register, overwriting the guest's R3.
    __asm volatile (
    "  cpsid i\n"
    "  push {lr}\n"              // Preserve the LR, which stores the EXC_RETURN value.
    "  ldr r0,=currGuest\n"      // Use R0 to point to the guest_regs array
    "  ldr r0,[r0]\n"
    "  ldr r0,[r0,#28]\n"
    "  add r0,r0,#4\n"
    
    "  stm r0,{r1-r12}\n"        // Store guest registers
    "  sub r0,r0,4\n"
    "  mrs r1,psp\n"             // Point R1 to the PSP (exception stack frame)
    "  ldr r2,[r1]\n"            // Retrieve guest's R0 from the exception stack frame (store it in R2)
    "  str r2, [r0]\n"           // Put guest's R0 into guest_regs
    "  ldr r2,[r1,#20]\n"        // Get the guest's LR off the exception stack frame
    "  str r2,[r0,#56]\n"        // Put the guest's LR into guest_regs
    );
    
    exceptionProcessor();
    
    __asm volatile (
    "  ldr r4,=currGuest\n"      // R4 <- currGuest->guest_regs
    "  ldr r4,[r4]\n"
    "  ldr r4,[r4,#28]\n"
    
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
static void executePrivilegedInstruction(uint16_t *offendingInstruction, struct inst *instruction)
{
    char *srcptr = (char*)&dummyfunc;
    memcpy(privexe, (char*)((uint32_t)srcptr & 0xfffffffe), 64); // Copy dummyfunc into privexe array. NOTE: the (srcptr & 0xfe) is a workaround because gcc always adds 1 to the address of a function pointer because
    *((uint16_t*)(privexe+14)) = 0xbf00; // Set the target instructions in privexe to NOPs
    *((uint16_t*)(privexe+16)) = 0xbf00; // Set the target instructions in privexe to NOPs

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
 * placeExceptionOnGuestStack
 *
 * Sets up a guest's stack to jump to an exception on return from the genericHandler.
 *
 */
static void placeExceptionOnGuestStack(struct vm *guest, uint32_t *psp, uint32_t exceptionNum) {
    uint32_t newLR; // New Link Reg, used when we're setting up to enter a guest exception handler
    uint32_t *newStackFrame; // Pointer to new stack frame when we're changing processor state (master/thread) or entering a guest ISR

    if(GUEST_IN_MASTER_MODE(guest)){
        newStackFrame = psp - 8;
        guest->MSP = newStackFrame;
        newLR = 0xFFFFFFF1;
    } else {
        newStackFrame = guest->MSP - 8;
        guest->MSP = newStackFrame;
        guest->PSP = psp;
        newLR = 0xFFFFFFFD;
    }

    // Set up a stack frame so when we return from this exception we will return to the guest's SysTick handler
    memcpy(newStackFrame,guest->guest_regs, 16);
    newStackFrame[4] = guest->guest_regs[12];
    newStackFrame[5] = newLR;
    *(newStackFrame+6) = (uint32_t)locateGuestISR(guest,exceptionNum) | 1; // Find the guest's PendSV handler
    newStackFrame[7] = (1<<24); // We're returning to an exception handler, so stack frame holds EPSR. Set Thumb bit to 1.
    guest->guest_regs[14] = newLR; // put newLR into guest_regs, since the stack frame version will be overwritten by generic_handler before returning to the guest. Note: I think the guest's LR should be saved on its stack, so this should be ok.

    SET_PROCESSOR_MODE_MASTER(guest); // Change to master mode since we're jumping to an exception processor
    SET_PROCESSOR_EXCEPTION(guest,exceptionNum);

    SET_CPU_PSP(newStackFrame);
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
static uint16_t *trackImpreciseBusFault(uint16_t *offendingInstruction, uint32_t busFaultAddress)
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
static uint16_t *findLastPrivilegedMemoryAccess(uint16_t *startingAddress)
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
static uint16_t *trackMRS(uint16_t *offendingInstruction){
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
 * trackCPS
 *
 * Starting at the address offendingInstruction, trace back through the program
 * trace sequentially in memory looking for CPS instructions. Return the
 * address of the CPS instruction.
 *
 */
static uint16_t *trackCPS(uint16_t *offendingInstruction){
    int i;
    struct inst instruction;

    for(i = 0; i < 5 ; i++){
        instDecode(&instruction, offendingInstruction-i);
        if((instruction.type == THUMB_TYPE_MISC_CPS)) {
            return offendingInstruction-i;
        }
    }
    return (uint16_t*)-1;
}

/*
 * trackWFE
 *
 * Starting at the address offendingInstruction, trace back through the program
 * trace sequentially in memory looking for MRS or MSR instructions. Return the
 * address of the WFE/WFI instruction.
 *
 */
static uint16_t *trackWFE(uint16_t *offendingInstruction){
    int i;
    struct inst instruction;

    for(i = 0; i < 5 ; i++){
        instDecode(&instruction, offendingInstruction-i);
        if((instruction.type == THUMB_TYPE_NOP_HINTS) && ((instruction.imm == NOP_HINT_WAIT_FOR_EVENT) || (instruction.imm == NOP_HINT_WAIT_FOR_INTERRUPT))) {
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
static int emulateSCBAccess(struct vm *guest, uint16_t *location, struct inst *instruction){

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
        } else { // Couldn't decode instruction...
            return -1;
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


/*
 * emulateSysTickAccess
 *
 * Emulate accesses to the SCB made by guest with instruction at location.
 *
 */
static int emulateSysTickAccess(struct vm *guest, uint16_t *location, struct inst *instruction){
    uint32_t *ea = effectiveAddress(instruction, guest);
    if(instruction->mnemonic[0] == 'L'){
    } else if(instruction->mnemonic[0] == 'S'){
        uint32_t storeVal;
        // Read the value to be stored from the decoded instruction.
        if(instruction->Rd != 0xff){
            storeVal = guest->guest_regs[instruction->Rd];
        } else if (instruction->Rt != 0xff) {
            storeVal = guest->guest_regs[instruction->Rt];
        }
        switch((uint32_t)ea){
        case (uint32_t)&CORTEXM7_SYST_CSR:
            guest->virtualSysTick->CSR = storeVal;
            break;
        case (uint32_t)&CORTEXM7_SYST_RVR:
            storeVal = storeVal / 240000;
            if(storeVal  == 0){
                storeVal = 2;
            }
            guest->virtualSysTick->RVR = storeVal;
            guest->virtualSysTick->CVR = storeVal; // Set up the CVR when we write to the RVR. This will start the counter.
            break;
        case (uint32_t)&CORTEXM7_SYST_CVR:
            storeVal = storeVal / 1000;
            if(storeVal  == 0){
                storeVal = 2;
            }
            guest->virtualSysTick->CVR = storeVal;
            break;
        case (uint32_t)&CORTEXM7_SYST_CALIB:
            guest->virtualSysTick->CALIB = storeVal;
            break;
        }
    }
    //executePrivilegedInstruction(location, instruction);
    return 0;
}

/*
 * emulateNVICAccess
 *
 * Emulate accesses to the NVIC made by guest with instruction at location. Our
 * strategy is to record the state modified by all writes to the NVIC and then
 * just let the instruction modify the actual hardware state. When this guest's
 * context is later restored, the context we save here will be restored by
 * updating the hardware registers.
 *
 */
static int emulateNVICAccess(struct vm *guest, uint16_t *location, struct inst *instruction){
    uint32_t *ea = effectiveAddress(instruction, guest);
    if(instruction->mnemonic[0] == 'L'){
    } else if(instruction->mnemonic[0] == 'S'){
        if((ea >= (uint32_t*)0xE000E100) && (ea <= (uint32_t*)0xE000E11C)){ // ISER access
        } else if ((ea >= (uint32_t*)0xE000E180) && (ea <= (uint32_t*)0xE000E19C)){ // ICER access
        } else if ((ea >= (uint32_t*)0xE000E200) && (ea <= (uint32_t*)0xE000E21c)){ // ISPR access
        } else if ((ea >= (uint32_t*)0xE000E280) && (ea <= (uint32_t*)0xE000E29c)){ // ICPR access
        } else if ((ea >= (uint32_t*)0xE000E300) && (ea <= (uint32_t*)0xE000E31c)){ // IABR access
        } else if ((ea >= (uint32_t*)0xE000E400) && (ea <= (uint32_t*)0xE000E4ef)){ // IPR access
            return 0;// Ignore writes to the IPRs
        }
    }
    executePrivilegedInstruction(location, instruction);
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
    uint16_t *offendingInstruction, *guestPC; // Address of instruction that caused the exception
    uint32_t *psp;
    uint32_t busFaultStatus,usageFaultStatus; // Fault status regs.
    uint32_t busFaultAddress; // Address in program mem that caused bus fault
    uint32_t *newStackFrame; // Pointer to new stack frame when we're changing processor state (master/thread) or entering a guest ISR
    struct inst instruction; // Instruction that caused the exception for bus faults and usage faults (emulated privileged instrs)
    uint32_t newLR; // New Link Reg, used when we're setting up to enter a guest exception handler
#if HERMES_ETHERNET_BRIDGE == 1
    uint32_t pkt_len;
#endif
    //static int count = 0;

    //count++;
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
            
            SYSTICK_INTERRUPT_ENABLE();

            SET_PROCESSOR_EXCEPTION(currGuest,0); // Indicate that the guest has returned from the exception it was handling.

            if(((uint32_t)guestPC & 0xe) != 0){ // Handles popping the PC off the stack and BX LR

                // If EXEC_RETURN word has 0x9 or 0xD in the low order byte, then we're returning to thread mode
                SET_PROCESSOR_MODE_THREAD(currGuest);
            
                SET_CPU_PSP(currGuest->PSP);

                SET_CPU_BASEPRI(currGuest->BASEPRI);

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
                
                SET_CPU_PSP(currGuest->MSP);
                
                SET_CPU_BASEPRI(currGuest->BASEPRI);
                
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

            if(offendingInstruction != (uint16_t*)-1) { // HACK ALERT!! Hack to avoid problem of unexplained access to 0xe000ed20

                uint32_t *ea = (uint32_t*)effectiveAddress(&instruction,currGuest);
                
                // TODO: Should emulate NVIC here too
                
                // If this is a load/store to the SCB, then we have special code to emulate SCB accesses.
                if((((ea >= (uint32_t*)0xe000ed00) && 
                    (ea < (uint32_t*)0xe000ed40)) ||
                    (ea == (uint32_t*)0xe000e008)) &&
                    ((instruction.type == THUMB_TYPE_LDSTREG) ||
                    (instruction.type == THUMB_TYPE_LDSTWORDBYTE) ||
                    (instruction.type == THUMB_TYPE_LDSTHALFWORD) ||
                    (instruction.type == THUMB_TYPE_LDSTSINGLE))){
                    if(emulateSCBAccess(currGuest, offendingInstruction, &instruction)){
                        // VM PANIC!!!
                        while(1);
                    }
                } else if(((ea >= (uint32_t*)0xE000E010) &&
                           (ea <= (uint32_t*)0xE000E01C)) &&
                          ((instruction.type == THUMB_TYPE_LDSTREG) ||
                          (instruction.type == THUMB_TYPE_LDSTWORDBYTE) ||
                          (instruction.type == THUMB_TYPE_LDSTHALFWORD) ||
                          (instruction.type == THUMB_TYPE_LDSTSINGLE))){
                    emulateSysTickAccess(currGuest, offendingInstruction, &instruction);
                } else if ( ((ea >= (uint32_t*)0xe000e100) && (ea <= (uint32_t*)0xe000e4ef)) || (ea == (uint32_t*)0xe000ef00)) {
                     emulateNVICAccess(currGuest, offendingInstruction, &instruction);
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
                // LATENCY TESTING
                instDecode(&instruction, guestPC);
                if(instruction.imm == 1){
                    asm("ldr r0,=0xe0001004\n"
                        "ldr %0, [r0]\n"
                        : "=r" (currGuest->guest_regs[0])
                        :
                        : "r0");
                        *(psp+6) += 2;
                        CORTEXM7_UFSR = 0xff;
                        break;
                }
                // LATENCY TESTING

                // Point the guest's PC past the illegal instruction and continue executing.
                *(psp+6) += 2;

                offendingInstruction = trackMRS(guestPC-1); // Find the MRS/MSR instruction assoc'd with the illegal instruction.
                if(offendingInstruction != (uint16_t*)-1){
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
                                SET_CPU_PSP(currGuest->MSP);
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
                            SET_CPU_BASEPRI(currGuest->guest_regs[instruction.Rn]);
                        }
                        break;
                    } // switch(instruction.imm)
                }// if(offendingInstruction != -1)

                // CPS instructions
                // When a guest executes a CPS instruction, we don't actually allow them to
                // modify the PRIMASK state because that would allow them to enable/disable
                // ALL interrupts systemwide, which effectively disable the hypervisor and
                // hang the system. Instead, CPSIE instructions set the BASEPRI register to
                // 0xff, which only disables interrupts with configurable priority. Usage
                // fault, bus fault, etc. are still allowed to execute as normal when
                // BASEPRI is set to 0xff, so the hypervisor can continue to work while the
                // guest code is free from exceptions.
                offendingInstruction = trackCPS(guestPC-1);
                // If the previous instruction was a CPS, then update the VM's PRIMASK
                if(offendingInstruction != (uint16_t*)-1){
                    instDecode(&instruction, offendingInstruction); // Decode the instruction
                    switch(instruction.mnemonic[4]){
                    case 'D': // Interrupt disable
                        SET_PRIMASK(currGuest);
                        // Disable interrupts for the guest by setting the BASEPRI to 0xff
                        __asm volatile(
                        "mov r0,0xff\n"
                        "msr basepri,r0\n"
                        :      // output
                        :      // input
                        :"r0"  // clobbered register
                        );
                        break;
                    case 'E': // Interrupt enable
                        CLEAR_PRIMASK(currGuest);
                        SET_CPU_BASEPRI(currGuest->BASEPRI);
                        break;
                    }
                }

                offendingInstruction = trackWFE(guestPC-1);
                // If the previous instruction was a wait for event or wait for interrupt, then the guest was trying to put the CPU to sleep. Un-schedule the guest and run the scheduler.
                if(offendingInstruction != (uint16_t*)-1){
                    struct vm *oldGuest = currGuest;


                    // Save the current guest's context.
                    if(GUEST_IN_MASTER_MODE(currGuest)){
                        currGuest->MSP = psp;
                    } else {
                        currGuest->PSP = psp;
                    }
                    // Remove the old guest from the active list and add it to the sleeping list.
                    if(currGuest->next != NULL){
                        currGuest = currGuest->next;
                    } else {
                        currGuest = guestList;
                    }

                    listRemove(oldGuest);
                    listAdd(&sleepingList, oldGuest);                  
                    SET_CPU_BASEPRI(currGuest->BASEPRI);
                    
                    // Install the new guest's PSP
                    if(GUEST_IN_MASTER_MODE(currGuest)){
                        SET_CPU_PSP(currGuest->MSP);
                    } else {
                        SET_CPU_PSP(currGuest->PSP);
                    }
#if 0
                    // re-read the PSP since it might have been changed by changing the currGuest
                    __asm
                    (
                    "  mrs %0,psp\n"
                    : "=r" (psp)                /* output */
                    :                           /* input */
                    :                           /* clobbered register */
                    );
#endif

                    
                    //hvScheduler(psp); // Run the scheduler to select a new guest

                }
                
                ///////////////////////////////////////////////////////////////
                // NOTE: Here we have an undefined instruction that does not
                // seem to be related to an MRS or CPS. We should probably
                // invoke the guest's exception vector.
            } else {
                while(1);
            }
            CORTEXM7_UFSR = 0xff; // Clear UFSR
            break;
        case 7:  // Reserved
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
            SET_CPU_PSP(newStackFrame);
            break;
        case 12: // Reserved for debug
        case 13: // Reserved
            break;
        case 14: // PendSV
            break;
        case 15: // SysTick
            // Clear all the VMs off the sleeping list
            do{
                struct vm *sleepIterator = sleepingList;
                struct vm* temp;
                while(sleepIterator != NULL){
                    // If this guest's SysTick module is not enabled, then we will not modify its CVR
                    if((sleepIterator->virtualSysTick->CSR & 1) == 0){
                        sleepIterator = sleepIterator->next;
                        continue;
                    }
                    if(--sleepIterator->virtualSysTick->CVR <= 0){ // If decrementing the CVR makes it zero, then we need to create an exception for that guest and reschedule it.
                        sleepIterator->virtualSysTick->CVR = sleepIterator->virtualSysTick->RVR + 1; // Reload the current value register
                        // Set up the stack for this guest to take a SysTick exception.
                        placeExceptionOnGuestStack(sleepIterator,
                                                   GUEST_IN_MASTER_MODE(sleepIterator) ? sleepIterator->MSP : sleepIterator->PSP,
                                                   ARM_CORTEX_M7_SYSTICK_ISR_NUM);

                        SET_CPU_BASEPRI(0x01);

                        temp = sleepIterator->next;
                        listRemove(sleepIterator);
                        listAdd(&guestList, sleepIterator);
                        sleepIterator = temp;
                    } else {
                        sleepIterator = sleepIterator->next;
                    }
                }
            }while(0);

            do{
                struct vm *guestIterator = guestList;
                while(guestIterator != NULL){
                    // If this guest's SysTick module is not enabled, then we will not modify its CVR
                    if((guestIterator->virtualSysTick->CSR & 1) == 0){
                        guestIterator = guestIterator->next;
                        continue;
                    }
                    if(--guestIterator->virtualSysTick->CVR <= 0){ // If decrementing the CVR makes it zero, then we need to create an exception for that guest and reschedule it.
                        guestIterator->virtualSysTick->CVR = guestIterator->virtualSysTick->RVR; // Reload the current value register
                        
                        // We need to preserve the PSP of the currGuest if we're going to set up an exception stack
                        // frame on it. This needs to be done before we set up the execption stack frame because
                        // that function forces the currGuest into master mode.
                        if((guestIterator == currGuest) && !GUEST_IN_MASTER_MODE(currGuest)){
                            currGuest->PSP = psp;
                        } else { /////////////////// BUG TRACKING ELSE BLOCK
                            currGuest->MSP = psp;
                        }
                        
                        // Set up the stack for this guest to take a SysTick exception.
                        placeExceptionOnGuestStack(guestIterator,
                                                   GUEST_IN_MASTER_MODE(guestIterator) ? guestIterator->MSP : guestIterator->PSP,
                                                   ARM_CORTEX_M7_SYSTICK_ISR_NUM);

                        SET_CPU_BASEPRI(0x01);

                        // placeExceptionOnGuestStack is going to update the guest's stack pointer.
                        // If that guest is the currently active one (currGuest), then we need to
                        // update psp for when we call hvScheduler() below.
                        if(guestIterator == currGuest){
                            // Save the PSP if necessary
                            psp = guestIterator->MSP;
                        } else { ///////////////////////// BUG TRACKING ELSE BLOCK
                        }
                        guestIterator = guestIterator->next;
                    } else {
                        guestIterator = guestIterator->next;
                    }
                }
            }while(0);

            hvScheduler(psp);

            return;
#if HERMES_INTERNAL_DMA == 1
        case SAME70_DMA_ISR_NUM:
            
            XDMAD_Handler(&lcdSpiDma);
            break;
#endif
#if HERMES_ETHERNET_BRIDGE == 1
        case SAME70_ETHERNET_ISR_NUM:
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
            // Save the current guest's context.
            if(GUEST_IN_MASTER_MODE(currGuest)){
                currGuest->MSP = psp;
            } else {
                currGuest->PSP = psp;
            }

            // Check if the exception is owned by a guest. Subtract 16 from exceptionNum because we don't store handler info for faults, which are the first 16 exceptions in the vector table.
            // This is an alternative to the hardcoded method below as presented in HotMobile 18
            if(intlist[exceptionNum-16].owner != NULL){
                currGuest = intlist[exceptionNum-16].owner;

                // If the guest was sleeping, put it back on the active list.
                listRemove(currGuest);
                listAdd(&guestList, currGuest);

                SET_CPU_BASEPRI(intlist[exceptionNum-16].priority);
            } else {
                // NOTE TO PROGRAMMER:
                // Your code got to this dark place because the CPU took an
                // exception that was not properly registered with Hermes. See
                // http://hermes.wings.cs.wisc.edu/docs/start.html 
                asm("bkpt #01");
            }

            // Install the new guest's PSP
            if(GUEST_IN_MASTER_MODE(currGuest)){
                SET_CPU_PSP(currGuest->MSP);
            } else {
                SET_CPU_PSP(currGuest->PSP);
            }
            // re-read the PSP since it might have been changed by changing the currGuest
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
            *(newStackFrame+6) = (uint32_t)locateGuestISR(currGuest,exceptionNum) | 1; // Find the guest's PendSV handler
            newStackFrame[7] = (1<<24); // We're returning to an exception handler, so stack frame holds EPSR. Set Thumb bit to 1.
            currGuest->guest_regs[14] = newLR; // put newLR into guest_regs, since the stack frame version will be overwritten by generic_handler before returning to the guest. Note: I think the guest's LR should be saved on its stack, so this should be ok.

            SET_PROCESSOR_MODE_MASTER(currGuest); // Change to master mode since we're jumping to an exception processor

            placeExceptionOnGuestStack(currGuest, psp, exceptionNum);
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
        "movs r0, 0xff\n"  // set the guest's BASEPRI to 0xff to disable interrupts while we're running the guest's ISR
        "msr basepri,r0\n"
        ://"=r"(currGuest->BASEPRI)   /* output */
        :"r"(newStackFrame)         /* input */
        :"r0"                       /* clobbered register */
        );


    }
    return;
}

