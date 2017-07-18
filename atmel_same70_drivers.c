
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

#include "board.h"

#include "uip.h"
#include "uip_arp.h"
#include "string.h"
#include "gmac_tapdev.h"
/*----------------------------------------------------------------------------
 *        Definitions
 *----------------------------------------------------------------------------*/
#define TX_BUFFERS         32     /** Must be a power of 2 */
#define RX_BUFFERS         32     /** Must be a power of 2 */
#define BUFFER_SIZE        1536
#define DUMMY_SIZE              2
#define DUMMY_BUFF_SIZE         512


#define GMAC_CAF_DISABLE  0
#define GMAC_CAF_ENABLE   1
#define GMAC_NBC_DISABLE  0
#define GMAC_NBC_ENABLE   1

/*----------------------------------------------------------------------------
 *        Variables
 *----------------------------------------------------------------------------*/

/* The PINs for GMAC */
static const Pin gmacPins[]   = {BOARD_GMAC_RUN_PINS};
static const Pin gmacResetPin = BOARD_GMAC_RESET_PIN;

extern sGmacd gGmacd __attribute__ ((aligned (32)));
extern GMacb gGmacb __attribute__ ((aligned (32)));

/** TX descriptors list */
__attribute__((__section__(".ram_nocache")))
static sGmacTxDescriptor gTxDs[TX_BUFFERS] __attribute__ ((aligned (32))),  gDummyTxDs[DUMMY_SIZE] __attribute__ ((aligned (32)));

/** RX descriptors list */
static sGmacRxDescriptor gRxDs[RX_BUFFERS] __attribute__ ((aligned (32))), gDummyRxDs[DUMMY_SIZE] __attribute__ ((aligned (32)));

/** TX callbacks list */
static fGmacdTransferCallback gTxCbs[TX_BUFFERS] __attribute__ ((aligned (32))), gDummyTxCbs[DUMMY_SIZE] __attribute__ ((aligned (32)));
/** Send Buffer */
/* Section 3.6 of AMBA 2.0 spec states that burst should not cross 1K Boundaries.
   Receive buffer manager writes are burst of 2 words => 3 lsb bits of the address
   shall be set to 0 */
static uint8_t pTxBuffer[TX_BUFFERS * BUFFER_SIZE] __attribute__ ((aligned (32))), pTxDummyBuffer[DUMMY_SIZE * DUMMY_BUFF_SIZE] __attribute__ ((aligned (32)));

/** Receive Buffer */
static uint8_t pRxBuffer[RX_BUFFERS * BUFFER_SIZE] __attribute__ ((aligned (32))), pRxDummyBuffer[DUMMY_SIZE * DUMMY_BUFF_SIZE] __attribute__ ((aligned (32)));

/* MAC address used for demo */
static uint8_t gGMacAddress[6] = {0x00, 0x45, 0x56, 0x78, 0x9a, 0xbc};



void gmac_tapdev_init_hermes(void)
{
	sGmacd    *pGmacd = &gGmacd;
	GMacb      *pGmacb = &gGmacb;
	sGmacInit Que0, Que;

	
	/* Init GMAC driver structure */
	memset(&Que0, 0, sizeof(Que0));
	Que0.bIsGem = 1;
	Que0.bDmaBurstLength = 4;
	Que0.pRxBuffer =pRxBuffer;
	Que0.pRxD = gRxDs;
	Que0.wRxBufferSize = BUFFER_SIZE;
	Que0.wRxSize = RX_BUFFERS;
	Que0.pTxBuffer = pTxBuffer;
	Que0.pTxD = gTxDs;
	Que0.wTxBufferSize = BUFFER_SIZE;
	Que0.wTxSize = TX_BUFFERS;
	Que0.pTxCb = gTxCbs;
	

	
	memset(&Que, 0, sizeof(Que));
	Que.bIsGem = 1;
	Que.bDmaBurstLength = 4;
	Que.pRxBuffer =pRxDummyBuffer;
	Que.pRxD = gDummyRxDs;
	Que.wRxBufferSize = DUMMY_BUFF_SIZE;
	Que.wRxSize = DUMMY_SIZE;
	Que.pTxBuffer = pTxDummyBuffer;
	Que.pTxD = gDummyTxDs;
	Que.wTxBufferSize = DUMMY_BUFF_SIZE;
	Que.wTxSize = DUMMY_SIZE;
	Que.pTxCb = gDummyTxCbs;
	
	/* Init GMAC driver structure */
	GMACD_Init(pGmacd, GMAC, ID_GMAC, GMAC_CAF_ENABLE, GMAC_NBC_DISABLE);
	GMACD_InitTransfer(pGmacd, &Que, GMAC_QUE_2);
	
	GMACD_InitTransfer(pGmacd, &Que, GMAC_QUE_1);
	
	GMACD_InitTransfer(pGmacd, &Que0, GMAC_QUE_0);
	GMAC_SetAddress(gGmacd.pHw, 0, gGMacAddress);

	/* Setup GMAC buffers and interrupts */
	/* Configure and enable interrupt on RC compare */
	NVIC_ClearPendingIRQ(GMAC_IRQn);
	NVIC_EnableIRQ(GMAC_IRQn);
	
	/* Init GMACB driver */
	GMACB_Init(pGmacb, pGmacd, BOARD_GMAC_PHY_ADDR);

	/* PHY initialize */
	if (!GMACB_InitPhy(pGmacb, BOARD_MCK,  &gmacResetPin, 1,  gmacPins, PIO_LISTSIZE( gmacPins )))
	{
		printf( "P: PHY Initialize ERROR!\n\r" ) ;
		return;
	}

	/* Auto Negotiate, work in RMII mode */
	if (!GMACB_AutoNegotiate(pGmacb))
	{
		printf( "P: Auto Negotiate ERROR!\n\r" ) ;
		return;
	}
	
	
	printf( "P: Link detected \n\r" ) ;
}
