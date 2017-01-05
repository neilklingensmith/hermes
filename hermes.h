/*

 * hv.h
 *
 * Created: 12/9/2016 12:02:34 PM
 *  Author: Neil Klingensmith
 */ 


#ifndef HV_H_
#define HV_H_


#define HV_STACK_SIZE 512


struct vm {
	struct vm *next;
	struct vm *prev;

	void *vectorTable;
};



void hvInit(void*) __attribute__((naked));


// ARM Cortex Specific Regs
#define SHCSR (*(uint32_t*)0xe000ed24)

#define CFSR (*(uint32_t*)0xe000ed28)  // Configurable Fault Status Register (Cortex M7 Peripherals > System Control Block > Configurable Fault Status Register)
#define MMFSR (*(uint8_t*)0xe000ed28)  // Memory Management Fault Status Register (Cortex M7 Peripherals > System Control Block > Configurable Fault Status Register)
#define BFSR (*(uint8_t*)0xe000ed29)   // Bus Fault Status Register (Cortex M7 Peripherals > System Control Block > Configurable Fault Status Register)
#define UFSR (*(uint16_t*)0xe000ed2a)  // Usage Fault Status Register (Cortex M7 Peripherals > System Control Block > Configurable Fault Status Register)

#define BFAR (*(uint32_t*)0xe000ed38)  // Bus fault address register (Cortex M7 Peripherals > System Control Block)
#endif /* HV_H_ */