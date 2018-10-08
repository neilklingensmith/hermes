/*
 * nalloc.h
 *
 * Created: 7/18/2017 10:31:25 AM
 *  Author: Neil Klingensmith
 */ 


#ifndef NALLOC_H_
#define NALLOC_H_

void memInit(void);
void *nalloc(int size);
void nfree(void *block);



#endif /* NALLOC_H_ */
