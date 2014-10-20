/**
 * @file
 * Ethernet Interface Skeleton
 *
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/*
 * This file is a skeleton for developing Ethernet network interface
 * drivers for lwIP. Add code to the low_level functions and do a
 * search-and-replace for the word "ethernetif" to replace it with
 * something that better describes your network interface.
 */

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/timers.h"
#include "netif/etharp.h"
#include "sys_arch.h"
#include "err.h"
#include "ethernetif.h"

#include "main.h"
#include "stm32f4x7_eth.h"
#include <string.h>


#define netifMTU												(1500)
#define netifINTERFACE_TASK_STACK_SIZE	(350)
#define netifINTERFACE_TASK_PRIORITY		(osPriorityRealtime)
#define netifGUARD_BLOCK_TIME						(250)

/* The time to block waiting for input. */
#define emacBLOCK_TIME_WAITING_FOR_INPUT	((u32_t) 100)

/* Define those to better describe your network interface. */
#define IFNAME0 's'
#define IFNAME1 't'

static struct netif *s_pxNetIf = NULL;
sys_sem_t s_xRxSemaphore;
sys_sem_t s_xTxSemaphore;

/* Ethernet Rx & Tx DMA Descriptors */
extern ETH_DMADESCTypeDef  DMARxDscrTab[ETH_RXBUFNB], DMATxDscrTab[ETH_TXBUFNB];

/* Ethernet Receive buffers  */
extern uint8_t Rx_Buff[ETH_RXBUFNB][ETH_RX_BUF_SIZE]; 

/* Ethernet Transmit buffers */
extern uint8_t Tx_Buff[ETH_TXBUFNB][ETH_TX_BUF_SIZE]; 

/* Global pointers to track current transmit and receive descriptors */
extern ETH_DMADESCTypeDef  *DMATxDescToSet;
extern ETH_DMADESCTypeDef  *DMARxDescToGet;

/* Global pointer for last received frame infos */
extern ETH_DMA_Rx_Frame_infos *DMA_RX_FRAME_infos;

static void ethernetif_input(void * pvParameters);
static void arp_timer(void *arg);

#if LWIP_PTP
static void ETH_PTPStart(uint32_t UpdateMethod);
#endif

u32_t ETH_PTPSubSecond2NanoSecond(u32_t SubSecondValue)
{
  uint64_t val = SubSecondValue * 1000000000ll;
  val >>=31;
  return val;
}


u32_t ETH_PTPNanoSecond2SubSecond(u32_t SubSecondValue)
{
  uint64_t val = SubSecondValue * 0x80000000ll;
  val /= 1000000000;
  return val;
}


void ETH_PTPTime_GetTime(struct ptptime_t * timestamp)
{
  timestamp->tv_nsec = ETH_PTPSubSecond2NanoSecond(ETH_GetPTPRegister(ETH_PTPTSLR));
  timestamp->tv_sec = ETH_GetPTPRegister(ETH_PTPTSHR);
}


/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static void low_level_init(struct netif *netif)
{
  uint32_t i;
 
  /* set netif MAC hardware address length */
  netif->hwaddr_len = ETHARP_HWADDR_LEN;
	
  /* set netif MAC hardware address */
  netif->hwaddr[0] =  MAC_ADDR0;
  netif->hwaddr[1] =  MAC_ADDR1;
  netif->hwaddr[2] =  MAC_ADDR2;
  netif->hwaddr[3] =  MAC_ADDR3;
  netif->hwaddr[4] =  MAC_ADDR4;
  netif->hwaddr[5] =  MAC_ADDR5;
  
  /* set netif maximum transfer unit */
  netif->mtu = 1500;

  /* Accept broadcast address and ARP traffic */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

#ifdef LWIP_IGMP
	/* Accept multicast traffic. */
	netif->flags |= NETIF_FLAG_IGMP;
#endif
  
  s_pxNetIf = netif;
 
  /* Create semaphores for managing ethernet resources. */
	sys_sem_new(&s_xRxSemaphore, 0);
	sys_sem_new(&s_xTxSemaphore, 1);

  /* Initialize MAC address in ethernet MAC */ 
  ETH_MACAddressConfig(ETH_MAC_Address0, netif->hwaddr); 
  
  /* Initialize Tx Descriptors list: Chain Mode */
  ETH_DMATxDescChainInit(DMATxDscrTab, &Tx_Buff[0][0], ETH_TXBUFNB);

  /* Initialize Rx Descriptors list: Chain Mode  */
  ETH_DMARxDescChainInit(DMARxDscrTab, &Rx_Buff[0][0], ETH_RXBUFNB);
  
  /* Enable Ethernet Rx interrrupt */
  { 
    for(i=0; i<ETH_RXBUFNB; i++)
    {
      ETH_DMARxDescReceiveITConfig(&DMARxDscrTab[i], ENABLE);
    }
  }

#ifdef CHECKSUM_BY_HARDWARE
  /* Enable the checksum insertion for the Tx frames */
  {
    for(i=0; i<ETH_TXBUFNB; i++)
    {
      ETH_DMATxDescChecksumInsertionConfig(&DMATxDscrTab[i], ETH_DMATxDesc_ChecksumTCPUDPICMPFull);
    }
  } 
#endif

#if LWIP_PTP
  /* Enable PTP Timestamping */
  ETH_PTPStart(ETH_PTP_FineUpdate);
  /* ETH_PTPStart(ETH_PTP_CoarseUpdate); */
#endif
  
  /* Create the task that handles the ETH_MAC */
	sys_thread_new((const char *) "Eth_if", ethernetif_input, NULL, netifINTERFACE_TASK_STACK_SIZE, netifINTERFACE_TASK_PRIORITY);
  
  /* Enable MAC and DMA transmission and reception */
  ETH_Start();   
}


/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become availale since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
  struct pbuf *q;
	err_t retval = ERR_OK;
  uint32_t l = 0;
  u8 *buffer ;
#if LWIP_PTP
	ETH_TimeStamp timeStamp;
#endif

	/* Take the ethernet mutex before sending the ethernet packet. */
	if (sys_arch_sem_wait(&s_xTxSemaphore, netifGUARD_BLOCK_TIME))
  {
		/* Point to the DMA descriptor buffer. */
    buffer = (u8 *)(DMATxDescToSet->Buffer1Addr);

		/* Fill in the DMA descriptor buffer. */
    for (q = p; q != NULL; q = q->next) 
    {
      memcpy((u8_t*)&buffer[l], q->payload, q->len);
      l = l + q->len;
    }

#if LWIP_PTP
		/* Transmit the packet filling in the packet timestamp. */
		if (ETH_Prepare_Transmit_Descriptors_TimeStamp(l, &timeStamp) != ETH_SUCCESS)
		{
			retval = ERR_IF;
		}
		else
		{
			/* Fill in the time stamp information. */
			p->time_sec = timeStamp.TimeStampHigh;
			p->time_nsec = ETH_PTPSubSecond2NanoSecond(timeStamp.TimeStampLow);
		}
#else
		/* Transmit the packet. */
    if (ETH_Prepare_Transmit_Descriptors(l) != ETH_SUCCESS)
		{
			retval = ERR_IF;
		}
#endif

		/* Release the ethernet mutex. */
		sys_sem_signal(&s_xTxSemaphore);
  }

  return retval;
}


/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
 */
static struct pbuf * low_level_input(struct netif *netif)
{
  struct pbuf *p, *q;
  u16_t len;
  uint32_t l=0,i =0;
  FrameTypeDef frame;
  u8 *buffer;
  __IO ETH_DMADESCTypeDef *DMARxNextDesc;
  
  p = NULL;
  
  /* Get received frame */
  frame = ETH_Get_Received_Frame_interrupt();
  
  /* check that frame has no error */
  if ((frame.descriptor->Status & ETH_DMARxDesc_ES) == (uint32_t)RESET)
  {
    
    /* Obtain the size of the packet and put it into the "len" variable. */
    len = frame.length;
    buffer = (u8 *)frame.buffer;

    /* We allocate a pbuf chain of pbufs from the pool. */
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
 
    /* Copy received frame from ethernet driver buffer to stack buffer */
    if (p != NULL)
    { 
      for (q = p; q != NULL; q = q->next)
      {
        memcpy((u8_t*)q->payload, (u8_t*)&buffer[l], q->len);
        l = l + q->len;
      } 

#if LWIP_PTP
			{
				p->time_sec = frame.descriptor->TimeStampHigh;
				p->time_nsec = ETH_PTPSubSecond2NanoSecond(frame.descriptor->TimeStampLow);
			}
#endif
    }
  }
  
  /* Release descriptors to DMA */
  /* Check if received frame with multiple DMA buffer segments */
  if (DMA_RX_FRAME_infos->Seg_Count > 1)
  {
    DMARxNextDesc = DMA_RX_FRAME_infos->FS_Rx_Desc;
  }
  else
  {
    DMARxNextDesc = frame.descriptor;
  }
  
  /* Set Own bit in Rx descriptors: gives the buffers back to DMA */
  for (i=0; i<DMA_RX_FRAME_infos->Seg_Count; i++)
  {  
    DMARxNextDesc->Status = ETH_DMARxDesc_OWN;
    DMARxNextDesc = (ETH_DMADESCTypeDef *)(DMARxNextDesc->Buffer2NextDescAddr);
  }
  
  /* Clear Segment_Count */
  DMA_RX_FRAME_infos->Seg_Count =0;
  
  
  /* When Rx Buffer unavailable flag is set: clear it and resume reception */
  if ((ETH->DMASR & ETH_DMASR_RBUS) != (u32)RESET)  
  {
    /* Clear RBUS ETHERNET DMA flag */
    ETH->DMASR = ETH_DMASR_RBUS;
      
    /* Resume DMA reception */
    ETH->DMARPDR = 0;
  }
  return p;
}


/**
 * This function is the ethernetif_input task, it is processed when a packet 
 * is ready to be read from the interface. It uses the function low_level_input() 
 * that should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
void ethernetif_input(void * pvParameters)
{
  struct pbuf *p;
  
  for( ;; )
  {
		if (sys_arch_sem_wait(&s_xRxSemaphore, emacBLOCK_TIME_WAITING_FOR_INPUT) != SYS_ARCH_TIMEOUT)
    {
			do
			{
				p = low_level_input(s_pxNetIf);
				if (p != NULL)
				{
					if (s_pxNetIf->input(p, s_pxNetIf) != ERR_OK)
					{
						pbuf_free(p);
						p=NULL;
					}
				}
			} while (p != NULL);			
    }
  }
}  
      

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t ethernetif_init(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;

  netif->output = etharp_output;
  netif->linkoutput = low_level_output;

  /* initialize the hardware */
  low_level_init(netif);
  
  etharp_init();
  sys_timeout(ARP_TMR_INTERVAL, arp_timer, NULL);

  return ERR_OK;
}


static void arp_timer(void *arg)
{
  etharp_tmr();
  sys_timeout(ARP_TMR_INTERVAL, arp_timer, NULL);
}


#if LWIP_PTP

/*******************************************************************************
* Function Name  : ETH_PTPStart
* Description    : Initialize timestamping ability of ETH
* Input          : UpdateMethod:
*                       ETH_PTP_FineUpdate   : Fine Update method
*                       ETH_PTP_CoarseUpdate : Coarse Update method 
* Output         : None
* Return         : None
*******************************************************************************/
static void ETH_PTPStart(uint32_t UpdateMethod)
{
  /* Check the parameters */
  assert_param(IS_ETH_PTP_UPDATE(UpdateMethod));

  /* Mask the Time stamp trigger interrupt by setting bit 9 in the MACIMR register. */
  ETH_MACITConfig(ETH_MAC_IT_TST, DISABLE);

  /* Program Time stamp register bit 0 to enable time stamping. */
  ETH_PTPTimeStampCmd(ENABLE);

  /* Program the Subsecond increment register based on the PTP clock frequency. */
  ETH_SetPTPSubSecondIncrement(ADJ_FREQ_BASE_INCREMENT); /* to achieve 20 ns accuracy, the value is ~ 43 */

  if (UpdateMethod == ETH_PTP_FineUpdate)
	{
    /* If you are using the Fine correction method, program the Time stamp addend register
     * and set Time stamp control register bit 5 (addend register update). */
    ETH_SetPTPTimeStampAddend(ADJ_FREQ_BASE_ADDEND);
    ETH_EnablePTPTimeStampAddend();

    /* Poll the Time stamp control register until bit 5 is cleared. */
    while(ETH_GetPTPFlagStatus(ETH_PTP_FLAG_TSARU) == SET);
  }

  /* To select the Fine correction method (if required),
   * program Time stamp control register  bit 1. */
  ETH_PTPUpdateMethodConfig(UpdateMethod);

  /* Program the Time stamp high update and Time stamp low update registers
   * with the appropriate time value. */
  ETH_SetPTPTimeStampUpdate(ETH_PTP_PositiveTime, 0, 0);

  /* Set Time stamp control register bit 2 (Time stamp init). */
  ETH_InitializePTPTimeStamp();

	/* The enhanced descriptor format is enabled and the descriptor size is
	 * increased to 32 bytes (8 DWORDS). This is required when time stamping 
	 * is activated above. */
	ETH_EnhancedDescriptorCmd(ENABLE);
	
  /* The Time stamp counter starts operation as soon as it is initialized
   * with the value written in the Time stamp update register. */
}

/*******************************************************************************
* Function Name  : ETH_PTPTimeStampAdjFreq
* Description    : Updates time stamp addend register
* Input          : Correction value in thousandth of ppm (Adj*10^9)
* Output         : None
* Return         : None
*******************************************************************************/
void ETH_PTPTime_AdjFreq(int32_t Adj)
{
	uint32_t addend;
	
	/* calculate the rate by which you want to speed up or slow down the system time
		 increments */
 
	/* precise */
	/*
	int64_t addend;
	addend = Adj;
	addend *= ADJ_FREQ_BASE_ADDEND;
	addend /= 1000000000-Adj;
	addend += ADJ_FREQ_BASE_ADDEND;
	*/

	/* 32bit estimation
	ADJ_LIMIT = ((1l<<63)/275/ADJ_FREQ_BASE_ADDEND) = 11258181 = 11 258 ppm*/
	if( Adj > 5120000) Adj = 5120000;
	if( Adj < -5120000) Adj = -5120000;

	addend = ((((275LL * Adj)>>8) * (ADJ_FREQ_BASE_ADDEND>>24))>>6) + ADJ_FREQ_BASE_ADDEND;
	
	/* Reprogram the Time stamp addend register with new Rate value and set ETH_TPTSCR */
	ETH_SetPTPTimeStampAddend((uint32_t)addend);
	ETH_EnablePTPTimeStampAddend();
}

/*******************************************************************************
* Function Name  : ETH_PTPTimeStampUpdateOffset
* Description    : Updates time base offset
* Input          : Time offset with sign
* Output         : None
* Return         : None
*******************************************************************************/
void ETH_PTPTime_UpdateOffset(struct ptptime_t * timeoffset)
{
	uint32_t Sign;
	uint32_t SecondValue;
	uint32_t NanoSecondValue;
	uint32_t SubSecondValue;
	uint32_t addend;

	/* determine sign and correct Second and Nanosecond values */
	if(timeoffset->tv_sec < 0 || (timeoffset->tv_sec == 0 && timeoffset->tv_nsec < 0))
	{
		Sign = ETH_PTP_NegativeTime;
		SecondValue = -timeoffset->tv_sec;
		NanoSecondValue = -timeoffset->tv_nsec;
	}
	else
	{
		Sign = ETH_PTP_PositiveTime;
		SecondValue = timeoffset->tv_sec;
		NanoSecondValue = timeoffset->tv_nsec;
	}

	/* convert nanosecond to subseconds */
	SubSecondValue = ETH_PTPNanoSecond2SubSecond(NanoSecondValue);

	/* read old addend register value*/
	addend = ETH_GetPTPRegister(ETH_PTPTSAR);

	while(ETH_GetPTPFlagStatus(ETH_PTP_FLAG_TSSTU) == SET);
	while(ETH_GetPTPFlagStatus(ETH_PTP_FLAG_TSSTI) == SET);

	/* Write the offset (positive or negative) in the Time stamp update high and low registers. */
	ETH_SetPTPTimeStampUpdate(Sign, SecondValue, SubSecondValue);

	/* Set bit 3 (TSSTU) in the Time stamp control register. */
	ETH_EnablePTPTimeStampUpdate();

	/* The value in the Time stamp update registers is added to or subtracted from the system */
	/* time when the TSSTU bit is cleared. */
	while(ETH_GetPTPFlagStatus(ETH_PTP_FLAG_TSSTU) == SET);      

	/* Write back old addend register value. */
	ETH_SetPTPTimeStampAddend(addend);
	ETH_EnablePTPTimeStampAddend();
}

/*******************************************************************************
* Function Name  : ETH_PTPTimeStampSetTime
* Description    : Initialize time base
* Input          : Time with sign
* Output         : None
* Return         : None
*******************************************************************************/
void ETH_PTPTime_SetTime(struct ptptime_t * timestamp)
{
	uint32_t Sign;
	uint32_t SecondValue;
	uint32_t NanoSecondValue;
	uint32_t SubSecondValue;

	/* determine sign and correct Second and Nanosecond values */
	if(timestamp->tv_sec < 0 || (timestamp->tv_sec == 0 && timestamp->tv_nsec < 0))
	{
		Sign = ETH_PTP_NegativeTime;
		SecondValue = -timestamp->tv_sec;
		NanoSecondValue = -timestamp->tv_nsec;
	}
	else
	{
		Sign = ETH_PTP_PositiveTime;
		SecondValue = timestamp->tv_sec;
		NanoSecondValue = timestamp->tv_nsec;
	}

	/* convert nanosecond to subseconds */
	SubSecondValue = ETH_PTPNanoSecond2SubSecond(NanoSecondValue);

	/* Write the offset (positive or negative) in the Time stamp update high and low registers. */
	ETH_SetPTPTimeStampUpdate(Sign, SecondValue, SubSecondValue);
	/* Set Time stamp control register bit 2 (Time stamp init). */
	ETH_InitializePTPTimeStamp();
	/* The Time stamp counter starts operation as soon as it is initialized
	 * with the value written in the Time stamp update register. */
	while(ETH_GetPTPFlagStatus(ETH_PTP_FLAG_TSSTI) == SET);
}

#endif /* LWIP_PTP */
