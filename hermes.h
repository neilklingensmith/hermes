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


#endif /* HV_H_ */