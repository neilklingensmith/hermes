/*

 * hv.h
 *
 * Created: 12/9/2016 12:02:34 PM
 *  Author: Neil Klingensmith
 */ 


#ifndef HV_H_
#define HV_H_


#include <stdint.h>


#define HV_STACK_SIZE 2048

// System control block for ARM Cortex M7 CPU
struct scb {
	uint32_t ACTLR;
	uint32_t CPUID;
	uint32_t ICSR;
	uint32_t VTOR;
	uint32_t AIRCR;
	uint32_t SCR;
	uint32_t CCR;
	uint32_t SHPR1;
	uint32_t SHPR2;
	uint32_t SHPR3;
	uint32_t SHCRS;
	uint32_t CFSR;
	uint32_t HFSR;
	uint32_t MMAR;
	uint32_t BFAR;
	uint32_t AFSR;
};

struct vm {
	struct vm *next;
	struct vm *prev;

	void *vectorTable;
	uint32_t *PSP;
	uint32_t *MSP;
	uint32_t PSR;
	uint32_t EXC_RETURN;
	uint32_t status;
	uint32_t *guest_regs;
	struct scb *SCB;
};



void hvInit(void*) __attribute__((naked));
void exceptionProcessor(void) ;


/*
 * VM Status - 32-bit Number 
 *
 *  31              24                                                                             0
 *  ----------------------------------------------------------------------------------------------------
 *  |    Exception   |                                                                          | MODE |
 *  ----------------------------------------------------------------------------------------------------
 *
 *  Exception - 8 Bits, currently active exception handler. 0 Indicates none
 *  Mode - 1 bit, Master (1) or Thread (0)
 *
 */

#define STATUS_MASK_MODE             1

#define STATUS_PROCESSOR_MODE_MASTER 1
#define STATUS_PROCESSOR_MODE_THREAD 0

#define SET_PROCESSOR_MODE_MASTER(g) (g->status |= STATUS_PROCESSOR_MODE_MASTER)   // Set guest g's execution mode to master
#define SET_PROCESSOR_MODE_THREAD(g) (g->status &= ~STATUS_PROCESSOR_MODE_MASTER)  // Set guest g's execution mode to thread

#define SET_PROCESSOR_EXCEPTION(g,e) (g->status = (g->status & 0x00ffffff) | (e<<24))  // Set guest g's currently executing exception to e
#define GET_PROCESSOR_EXCEPTION(g) ((g->status >> 24) & 0xff)

#define GUEST_IN_MASTER_MODE(a)      (a->status & STATUS_PROCESSOR_MODE_MASTER) // Macro to check if a guest is in master mode

///////////////////////////////////////////////
// ARM Cortex Specific Regs

#define SHCSR (*(uint32_t*)0xe000ed24)

#define CFSR (*(uint32_t*)0xe000ed28)  // Configurable Fault Status Register (Cortex M7 Peripherals > System Control Block > Configurable Fault Status Register)
#define MMFSR (*(uint8_t*)0xe000ed28)  // Memory Management Fault Status Register (Cortex M7 Peripherals > System Control Block > Configurable Fault Status Register)
#define BFSR (*(uint8_t*)0xe000ed29)   // Bus Fault Status Register (Cortex M7 Peripherals > System Control Block > Configurable Fault Status Register)
#define UFSR (*(uint16_t*)0xe000ed2a)  // Usage Fault Status Register (Cortex M7 Peripherals > System Control Block > Configurable Fault Status Register)

#define BFAR (*(uint32_t*)0xe000ed38)  // Bus fault address register (Cortex M7 Peripherals > System Control Block)
#define ABFSR (*(uint32_t*)0xe000efa8) // Aux bus fault status register (Cortex M7 Peripherals > Access Control)
#define ACTLR (*(uint32_t*)0xe000e008)   // Auxiliary control register (Cortex M7 Peripherals > System Control Block)

#define CORTEXM7_CCR (*(uint32_t*)0xe000ed14)


#define BFSR_IMPRECISEERR_MASK 0x00000004
#define BFSR_BFARVALID_MASK    0x00000080



///////////////////////////////////////////////
// ARM Cortex ISR numbers

#define ARM_CORTEX_M7_HARDFAULT_ISR_NUM       3
#define ARM_CORTEX_M7_MEMMANAGEFAULT_ISR_NUM  4
#define ARM_CORTEX_M7_BUSFAULT_ISR_NUM        5
#define ARM_CORTEX_M7_USAGEFAULT_ISR_NUM      6
#define ARM_CORTEX_M7_PENDSV_ISR_NUM          14
#define ARM_CORTEX_M7_SYSTICK_ISR_NUM         15


#endif /* HV_H_ */