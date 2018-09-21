
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

#ifndef VIRT_ETH_H_
#define VIRT_ETH_H_


#define HERMES_VIRTUAL_ETH_EXCEPTION_NUM (55+16)
#define ETH_BUF_SIZE 1500

struct rxBufDesc {
    union veRxAddr {
        uint32_t val;
        struct veRxAddrBM {
            uint32_t bOwnership: 1, // User clear, GMAC set this to one once
            // it has successfully written a frame to
            // memory
            bWrap: 1,      // Marks last descriptor in receive buffer
            addrDW: 30;    // Address in number of DW
        } bm;
    } addr;                         // Address, Wrap & Ownership
    uint16_t length;
};

struct virt_eth {
    struct virt_eth *next;
    struct virt_eth *prev;
    struct vm *guest;      // Owner of this interface
    struct rxBufDesc *rxBufDescList;
    struct rxBufDesc *currRxBufWrite;
    struct rxBufDesc *currRxBufRead;
    uint8_t macAddress[6]; // MAC of this interface
};


int createVirtualNetworkInterface(char *mac, struct rxBufDesc *rxBufDescList);


#endif /* VIRT_ETH_H_ */
