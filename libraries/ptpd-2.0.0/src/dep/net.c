/* net.c */

#include "../ptpd.h"

/* Initialize network queue. */
static void netQInit(BufQueue *queue)
{
	queue->head = 0;
	queue->tail = 0;
	sys_mutex_new(&queue->mutex);
}

/* Put data to the network queue. */
static bool netQPut(BufQueue *queue, void *pbuf)
{
	bool retval = FALSE;

	sys_mutex_lock(&queue->mutex);

	// Is there room on the queue for the buffer?
	if (((queue->head + 1) & PBUF_QUEUE_MASK) != queue->tail)
	{
		// Place the buffer in the queue.
		queue->head = (queue->head + 1) & PBUF_QUEUE_MASK;
		queue->pbuf[queue->head] = pbuf;
		retval = TRUE;
	}

	sys_mutex_unlock(&queue->mutex);

	return retval;
}

/* Get data from the network queue. */
static void *netQGet(BufQueue *queue)
{
	void *pbuf = NULL;

	sys_mutex_lock(&queue->mutex);

	// Is there a buffer on the queue?
	if (queue->tail != queue->head)
	{
		// Get the buffer from the queue.
		queue->tail = (queue->tail + 1) & PBUF_QUEUE_MASK;
		pbuf = queue->pbuf[queue->tail];
	}

	sys_mutex_unlock(&queue->mutex);

	return pbuf;
}

/* Free any remaining pbufs in the queue. */
static void netQEmpty(BufQueue *queue)
{
	sys_mutex_lock(&queue->mutex);

	// Free each remaining buffer in the queue.
	while (queue->tail != queue->head)
	{
		// Get the buffer from the queue.
		queue->tail = (queue->tail + 1) & PBUF_QUEUE_MASK;
		pbuf_free(queue->pbuf[queue->tail]);
	}
	
	sys_mutex_unlock(&queue->mutex);
}

/* Check if something is in the queue */
static bool netQCheck(BufQueue  *queue)
{
	bool  retval = FALSE;

	sys_mutex_lock(&queue->mutex);

	if (queue->tail != queue->head) retval = TRUE;

	sys_mutex_unlock(&queue->mutex);

	return retval;
}

/* Shut down  the UDP and network stuff */
bool netShutdown(NetPath *netPath)
{
	struct ip_addr multicastAaddr;

	DBG("netShutdown\n");

	/* leave multicast group */
	multicastAaddr.addr = netPath->multicastAddr;
	igmp_leavegroup(IP_ADDR_ANY, &multicastAaddr);

	/* Disconnect and close the Event UDP interface */
	if (netPath->eventPcb)
	{
		udp_disconnect(netPath->eventPcb);
		udp_remove(netPath->eventPcb);
		netPath->eventPcb = NULL;
	}

	/* Disconnect and close the General UDP interface */
	if (netPath->generalPcb)
	{
		udp_disconnect(netPath->generalPcb);
		udp_remove(netPath->generalPcb);
		netPath->generalPcb = NULL;
	}

	/* Clear the network addresses. */
	netPath->multicastAddr = 0;
	netPath->unicastAddr = 0;

	/* Return a success code. */
	return TRUE;
}

/* Find interface to  be used.  uuid should be filled with MAC address of the interface.
	 Will return the IPv4 address of  the interface. */
static int32_t findIface(const octet_t *ifaceName, octet_t *uuid, NetPath *netPath)
{
	struct netif *iface;

	iface = netif_default;
	memcpy(uuid, iface->hwaddr, iface->hwaddr_len);

	return iface->ip_addr.addr;
}

/* Process an incoming message on the Event port. */
static void netRecvEventCallback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
																 struct ip_addr *addr, u16_t port)
{
	NetPath *netPath = (NetPath *) arg;

	/* Place the incoming message on the Event Port QUEUE. */
	if (!netQPut(&netPath->eventQ, p))
	{
		pbuf_free(p);
		ERROR("netRecvEventCallback: queue full\n");
		return;
	}

	/* Alert the PTP thread there is now something to do. */
	ptpd_alert();
}

/* Process an incoming message on the General port. */
static void netRecvGeneralCallback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
																	 struct ip_addr *addr, u16_t port)
{
	NetPath *netPath = (NetPath *) arg;

	/* Place the incoming message on the Event Port QUEUE. */
	if (!netQPut(&netPath->generalQ, p))
	{
		pbuf_free(p);
		ERROR("netRecvGeneralCallback: queue full\n");
		return;
	}

	/* Alert the PTP thread there is now something to do. */
	ptpd_alert();
}

/* Start  all of the UDP stuff */
bool netInit(NetPath *netPath, PtpClock *ptpClock)
{
	struct in_addr netAddr;
	struct ip_addr interfaceAddr;
	char addrStr[NET_ADDRESS_LENGTH];

	DBG("netInit\n");

	/* Initialize the buffer queues. */
	netQInit(&netPath->eventQ);
	netQInit(&netPath->generalQ);

	/* Find a network interface */
	interfaceAddr.addr = findIface(ptpClock->rtOpts->ifaceName, ptpClock->portUuidField, netPath);
	if (!(interfaceAddr.addr))
	{
			ERROR("netInit: Failed to find interface address\n");
			goto fail01;
	}

	/* Open lwIP raw udp interfaces for the event port. */
	netPath->eventPcb = udp_new();
	if (NULL == netPath->eventPcb)
	{
			ERROR("netInit: Failed to open Event UDP PCB\n");
			goto fail02;
	}

	/* Open lwIP raw udp interfaces for the general port. */
	netPath->generalPcb = udp_new();
	if (NULL == netPath->generalPcb)
	{
			ERROR("netInit: Failed to open General UDP PCB\n");
			goto fail03;
	}

	/* Configure network (broadcast/unicast) addresses. */
	netPath->unicastAddr = 0; /* disable unicast */

	/* Init General multicast IP address */
	memcpy(addrStr, DEFAULT_PTP_DOMAIN_ADDRESS, NET_ADDRESS_LENGTH);
	if (!inet_aton(addrStr, &netAddr))
	{
			ERROR("netInit: failed to encode multi-cast address: %s\n", addrStr);
			goto fail04;
	}
	netPath->multicastAddr = netAddr.s_addr;

	/* Join multicast group (for receiving) on specified interface */
	igmp_joingroup(&interfaceAddr, (struct ip_addr *)&netAddr);

	/* Init Peer multicast IP address */
	memcpy(addrStr, PEER_PTP_DOMAIN_ADDRESS, NET_ADDRESS_LENGTH);
	if (!inet_aton(addrStr, &netAddr))
	{
			ERROR("netInit: failed to encode peer multi-cast address: %s\n", addrStr);
			goto fail04;
	}
	netPath->peerMulticastAddr = netAddr.s_addr;

	/* Join peer multicast group (for receiving) on specified interface */
	igmp_joingroup(&interfaceAddr, (struct ip_addr *) &netAddr);

	/* Multicast send only on specified interface. */
	netPath->eventPcb->multicast_ip.addr = netPath->multicastAddr;
	netPath->generalPcb->multicast_ip.addr = netPath->multicastAddr;

	/* Establish the appropriate UDP bindings/connections for events. */
	udp_recv(netPath->eventPcb, netRecvEventCallback, netPath);
	udp_bind(netPath->eventPcb, IP_ADDR_ANY, PTP_EVENT_PORT);
	/*  udp_connect(netPath->eventPcb, &netAddr, PTP_EVENT_PORT); */

	/* Establish the appropriate UDP bindings/connections for general. */
	udp_recv(netPath->generalPcb, netRecvGeneralCallback, netPath);
	udp_bind(netPath->generalPcb, IP_ADDR_ANY, PTP_GENERAL_PORT);
	/*  udp_connect(netPath->generalPcb, &netAddr, PTP_GENERAL_PORT); */

	/* Return a success code. */
	return TRUE;

fail04:
	udp_remove(netPath->generalPcb);
fail03:
	udp_remove(netPath->eventPcb);
fail02:
fail01:
	return FALSE;
}

/* Wait for a packet  to come in on either port.  For now, there is no wait.
 * Simply check to  see if a packet is available on either port and return 1,
 *  otherwise return 0. */
int32_t netSelect(NetPath *netPath, const TimeInternal *timeout)
{
	/* Check the packet queues.  If there is data, return TRUE. */
	if (netQCheck(&netPath->eventQ) || netQCheck(&netPath->generalQ)) return 1;

	return 0;
}

/* Delete all waiting packets in event queue. */
void netEmptyEventQ(NetPath *netPath)
{
	netQEmpty(&netPath->eventQ);
}

static ssize_t netRecv(octet_t *buf, TimeInternal *time, BufQueue *msgQueue)
{
	int i;
	int j;
	u16_t length;
	struct pbuf *p;
	struct pbuf *pcopy;

	/* Get the next buffer from the queue. */
	if ((p = (struct pbuf*) netQGet(msgQueue)) == NULL)
	{
		return 0;
	}

	/* Verify that we have enough space to store the contents. */
	if (p->tot_len > PACKET_SIZE)
	{
		ERROR("netRecv: received truncated message\n");
		pbuf_free(p);
		return 0;
	}

	/* Verify there is contents to copy. */
	if (p->tot_len == 0)
	{
		ERROR("netRecv: received empty packet\n");
		pbuf_free(p);
		return 0;
	}

	if (time != NULL)
	{
#if LWIP_PTP
		time->seconds = p->time_sec;
		time->nanoseconds = p->time_nsec;
#else
		getTime(time);
#endif
	}

	/* Get the length of the buffer to copy. */
	length = p->tot_len;

	/* Copy the pbuf payload into the buffer. */
	pcopy = p;
	j = 0;
	for (i = 0; i < length; i++)
	{
		// Copy the next byte in the payload.
		buf[i] = ((u8_t *)pcopy->payload)[j++];

		// Skip to the next buffer in the payload?
		if (j == pcopy->len)
		{
			// Move to the next buffer.
			pcopy = pcopy->next;
			j = 0;
		}
	}

	/* Free up the pbuf (chain). */
	pbuf_free(p);

	return length;
}

ssize_t netRecvEvent(NetPath *netPath, octet_t *buf, TimeInternal *time)
{
	return netRecv(buf, time, &netPath->eventQ);
}

ssize_t netRecvGeneral(NetPath *netPath, octet_t *buf, TimeInternal *time)
{
	return netRecv(buf, time, &netPath->generalQ);
}

static ssize_t netSend(const octet_t *buf, int16_t  length, TimeInternal *time, const int32_t * addr, struct udp_pcb * pcb)
{
	err_t result;
	struct pbuf * p;

	/* Allocate the tx pbuf based on the current size. */
	p = pbuf_alloc(PBUF_TRANSPORT, length, PBUF_RAM);
	if (NULL == p)
	{
		ERROR("netSend: Failed to allocate Tx Buffer\n");
		goto fail01;
	}

	/* Copy the incoming data into the pbuf payload. */
	result = pbuf_take(p, buf, length);
	if (ERR_OK != result)
	{
		ERROR("netSend: Failed to copy data to Pbuf (%d)\n", result);
		goto fail02;
	}

	/* send the buffer. */
	result = udp_sendto(pcb, p, (void *)addr, pcb->local_port);
	if (ERR_OK != result)
	{
		ERROR("netSend: Failed to send data (%d)\n", result);
		goto fail02;
	}

	if (time != NULL)
	{
#if LWIP_PTP
		time->seconds = p->time_sec;
		time->nanoseconds = p->time_nsec;
#else
		/* TODO: use of loopback mode */
		/*
		time->seconds = 0;
		time->nanoseconds = 0;
		*/
		getTime(time);
#endif
		DBGV("netSend: %d sec %d nsec\n", time->seconds, time->nanoseconds);
	} else {
		DBGV("netSend\n");
	}


fail02:
	pbuf_free(p);

fail01:
	return length;

	/*  return (0 == result) ? length : 0; */
}

ssize_t netSendEvent(NetPath *netPath, const octet_t *buf, int16_t  length, TimeInternal *time)
{
	return netSend(buf, length, time, &netPath->multicastAddr, netPath->eventPcb);
}

ssize_t netSendGeneral(NetPath *netPath, const octet_t *buf, int16_t  length)
{
	return netSend(buf, length, NULL, &netPath->multicastAddr, netPath->generalPcb);
}

ssize_t netSendPeerGeneral(NetPath *netPath, const octet_t *buf, int16_t  length)
{
	return netSend(buf, length, NULL, &netPath->peerMulticastAddr, netPath->generalPcb);
}

ssize_t netSendPeerEvent(NetPath *netPath, const octet_t *buf, int16_t  length, TimeInternal* time)
{
	return netSend(buf, length, time, &netPath->peerMulticastAddr, netPath->eventPcb);
}
