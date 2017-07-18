
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
#include "virt_eth.h"
#include <stdint.h>
#include <string.h>

#ifdef HERMES_ETHERNET_BRIDGE

uint8_t baseMacAddress[6] = {0x00, 0x15, 0x8d, 0x78, 0x9b, 0x40};
struct virt_eth *ifList = NULL;
extern struct vm *currGuest;

/*
 * createVirtualNetworkInterface
 *
 * Creates a virtual network interface. If mac is set to NULL, then a new MAC is chosen for it semi-randomly.
 *
 */
int createVirtualNetworkInterface(char *mac, struct rxBufDesc *rxBufDescList) {
	struct virt_eth *newIf = nalloc(sizeof(struct virt_eth));

	memset(newIf,0,sizeof(struct virt_eth));

	newIf->guest = currGuest;
	// If user did not specify a MAC address, we assume he wants us to pick one for him.
	if(mac == NULL){
		memcpy(newIf->macAddress,baseMacAddress,6);
		baseMacAddress[5]++;
	} else {
		memcpy(newIf->macAddress,mac,6);
	}
	
	newIf->currRxBufWrite = rxBufDescList;
	newIf->currRxBufRead = rxBufDescList;
	newIf->rxBufDescList = rxBufDescList;
	
	// Link new interface into the list
	listAdd((struct listElement **)&ifList, (struct listElement *)newIf);
	
	return 0;
}


uint32_t virtEthRead(uint8_t *buffer, uint32_t len) {
	struct virt_eth *iterator = ifList;
	
	// Find the virtual interface assoc'd with this guest
	do{
		if(iterator->guest == currGuest){
			break;
		}
		iterator = iterator->next;
	}while(iterator != NULL);
	
	// Check if new data is available in the buffer
	if(iterator->currRxBufRead->addr.bm.bOwnership == 1){
		if(iterator->currRxBufRead->length < len){
			len = iterator->currRxBufRead->length;
		}
		memcpy(buffer,iterator->currRxBufRead->addr.bm.addrDW,len);
		iterator->currRxBufRead->addr.bm.bOwnership = 0; // Set ownership bit to 0 to indicate that we have read the frame.
		
		// Point the read pointer to the next element in the Rx buffer
		if(iterator->currRxBufRead->addr.bm.bWrap == 0){
			iterator->currRxBufRead++; // Increment read pointer			
		} else {
			iterator->currRxBufRead = iterator->rxBufDescList; // Wrap read pointer
		}
		
		return len;
	}
	return 0;
}




#endif