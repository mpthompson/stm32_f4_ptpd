#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

#include <stdio.h>
#include <stdlib.h>

/* Use printf as platform diagnostic function. */
#define LWIP_PLATFORM_DIAG(message) printf message;

/* Use rand as platform random number function. */
#define LWIP_RAND rand

/* SYS_LIGHTWEIGHT_PROT==1: if you want inter-task protection for certain
 * critical regions during buffer allocation, deallocation and memory
 * allocation and deallocation. */
#define SYS_LIGHTWEIGHT_PROT    0

#define ETHARP_TRUST_IP_MAC     0
#define IP_REASSEMBLY           0
#define IP_FRAG                 0
#define ARP_QUEUEING            0
#define TCP_LISTEN_BACKLOG      1

/* NO_SYS==1: Provides VERY minimal functionality. Otherwise,
 * use lwIP facilities. */
#define NO_SYS                  0

/* System millisecond delay function. */
#define sys_msleep							osDelay

/* ---------- Memory options ---------- */

/* MEM_ALIGNMENT: should be set to the alignment of the CPU for which
 * lwIP is compiled. 4 byte alignment -> define MEM_ALIGNMENT to 4, 2
 * byte alignment -> define MEM_ALIGNMENT to 2. */
#define MEM_ALIGNMENT           4

/* MEM_SIZE: the size of the heap memory. If the application will send
 * a lot of data that needs to be copied, this should be set high. */
#define MEM_SIZE                				(4 * 1024)

/* ---------- Internal Memory Pool Sizes ---------- */

/* MEMP_NUM_PBUF: the number of memp struct pbufs (used for PBUF_ROM and PBUF_REF).
 * If the application sends a lot of data out of ROM (or other static memory),
 * this should be set high. */
#define MEMP_NUM_PBUF                   16

/* MEMP_NUM_RAW_PCB: Number of raw connection PCBs
 * (requires the LWIP_RAW option) */
#define MEMP_NUM_RAW_PCB                4

/* MEMP_NUM_UDP_PCB: the number of UDP protocol control blocks. One
 * per active UDP "connection". (requires the LWIP_UDP option) */
#define MEMP_NUM_UDP_PCB                8

/* MEMP_NUM_TCP_PCB: the number of simulatenously active TCP connections.
 * (requires the LWIP_TCP option) */
#define MEMP_NUM_TCP_PCB                4

/* MEMP_NUM_TCP_PCB_LISTEN: the number of listening TCP connections.
 * (requires the LWIP_TCP option) */
#define MEMP_NUM_TCP_PCB_LISTEN         4

/* MEMP_NUM_TCP_SEG: the number of simultaneously queued TCP segments.
 * (requires the LWIP_TCP option) */
#define MEMP_NUM_TCP_SEG                16

/* MEMP_NUM_REASSDATA: the number of simultaneously IP packets queued for
 * reassembly (whole packets, not fragments!) */
#define MEMP_NUM_REASSDATA              5

/* MEMP_NUM_ARP_QUEUE: the number of simulateously queued outgoing
 * packets (pbufs) that are waiting for an ARP request (to resolve
 * their destination address) to finish. (requires the ARP_QUEUEING 
 * option) */
#define MEMP_NUM_ARP_QUEUE              30

/* MEMP_NUM_IGMP_GROUP: The number of multicast groups whose network interfaces
 * can be members at the same time (one per netif - allsystems group -, plus one
 * per netif membership). (requires the LWIP_IGMP option) */
#define MEMP_NUM_IGMP_GROUP             8

/* MEMP_NUM_SYS_TIMEOUT: the number of simulateously active timeouts.
 * (requires NO_SYS==0) */
#define MEMP_NUM_SYS_TIMEOUT            6

/* MEMP_NUM_NETBUF: the number of struct netbufs.
 * (only needed if you use the sequential API, like api_lib.c) */
#define MEMP_NUM_NETBUF                 8

/* MEMP_NUM_NETCONN: the number of struct netconns.  MEMP_NUM_NETCONN should be 
 * less than the sum of MEMP_NUM_{TCP,RAW,UDP}_PCB+MEMP_NUM_TCP_PCB_LISTEN
 * (only needed if you use the sequential API, like api_lib.c) */
#define MEMP_NUM_NETCONN                20

/* MEMP_NUM_TCPIP_MSG_API: the number of struct tcpip_msg, which are used
 * for callback/timeout API communication. (only needed if you use tcpip.c) */
#define MEMP_NUM_TCPIP_MSG_API          8

/* MEMP_NUM_TCPIP_MSG_INPKT: the number of struct tcpip_msg, which are used
 * for incoming packets. (only needed if you use tcpip.c) */
#define MEMP_NUM_TCPIP_MSG_INPKT        8

/* PBUF_POOL_SIZE: the number of buffers in the pbuf pool. */
#define PBUF_POOL_SIZE                  16

/* ---------- Pbuf options ---------- */

/* PBUF_POOL_SIZE: the number of buffers in the pbuf pool. */
#define PBUF_POOL_SIZE          16

/* PBUF_LINK_HLEN: the number of bytes that should be allocated for a
 * link level header. The default is 14, the standard value for
 * Ethernet. */
#define PBUF_LINK_HLEN          14

/* PBUF_POOL_BUFSIZE: the size of each pbuf in the pbuf pool. The default is
 * designed to accomodate single full size TCP frame in one pbuf, including
 * TCP_MSS, IP header, and link header. */
#define PBUF_POOL_BUFSIZE       LWIP_MEM_ALIGN_SIZE(TCP_MSS+40+PBUF_LINK_HLEN)

/* ---------- UDP options ---------- */

/* LWIP_UDP==1: Turn on UDP. */
#define LWIP_UDP                        1

/* LWIP_UDPLITE==1: Turn on UDP-Lite. (Requires LWIP_UDP) */
#define LWIP_UDPLITE                    0

/* UDP_TTL: Default Time-To-Live value. */
#define UDP_TTL                         (IP_DEFAULT_TTL)

/* LWIP_NETBUF_RECVINFO==1: append destination addr and port to every netbuf. */
#define LWIP_NETBUF_RECVINFO            0

/* ---------- TCP options ---------- */

/* LWIP_TCP==1: Turn on TCP. */
#define LWIP_TCP                				1

/* TCP_TTL: Default Time-To-Live value. */
#define TCP_TTL                 				(IP_DEFAULT_TTL)

/* Controls if TCP should queue segments that arrive out of
 * order. Define to 0 if your device is low on memory. */
#define TCP_QUEUE_OOSEQ         				0

/* TCP Maximum segment size. 
 * TCP_MSS = (Ethernet MTU - IP header size - TCP header size) */
#define TCP_MSS                 				(1500 - 40)

/* TCP sender buffer space (bytes). */
#define TCP_SND_BUF             				(4 * TCP_MSS)

/* TCP_SND_QUEUELEN: TCP sender buffer space (pbufs). This must be at least
 * as much as (2 * TCP_SND_BUF/TCP_MSS) for things to work. */
#define TCP_SND_QUEUELEN        				((2 * TCP_SND_BUF) / TCP_MSS)

/* TCP receive window. */
#define TCP_WND                 				(4 * TCP_MSS)

/* ---------- ICMP options ---------- */

/* LWIP_ICMP==1: Enable ICMP module inside the IP stack.
 * Be careful, disable that make your product non-compliant to RFC1122 */
#define LWIP_ICMP                       1

/* ---------- DHCP options ---------- */

/* Define LWIP_DHCP to 1 if you want DHCP configuration of
 * interfaces. DHCP is not implemented in lwIP 0.5.1, however, so
 * turning this on does currently not work. */
#define LWIP_DHCP               				1

/* ---------- IGMP options ---------- */

/* LWIP_IGMP==1: Turn on IGMP module. */
#define LWIP_IGMP                       1

/* ---------- PTP options ---------- */
#define LWIP_PTP												1

/* ---------- Statistics options ---------- */

/* LWIP_STATS==1: Enable statistics collection in lwip_stats. */
#define LWIP_STATS                      0
#define LWIP_PROVIDE_ERRNO 							1

/* ---------- Checksum options ---------- */

/* The STM32F4x7 allows computing and verifying the IP, UDP, TCP and ICMP checksums by hardware:
 * - To use this feature let the following define uncommented.
 * - To disable it and process by CPU comment the  the checksum. */
#define CHECKSUM_BY_HARDWARE 

#ifdef CHECKSUM_BY_HARDWARE
  /* CHECKSUM_GEN_IP==0: Generate checksums by hardware for outgoing IP packets.*/
  #define CHECKSUM_GEN_IP                 0
  /* CHECKSUM_GEN_UDP==0: Generate checksums by hardware for outgoing UDP packets.*/
  #define CHECKSUM_GEN_UDP                0
  /* CHECKSUM_GEN_TCP==0: Generate checksums by hardware for outgoing TCP packets.*/
  #define CHECKSUM_GEN_TCP                0 
  /* CHECKSUM_GEN_ICMP==0: Generate checksums by hardware for outgoing ICMP packets.*/
  #define CHECKSUM_GEN_ICMP               0
  /* CHECKSUM_CHECK_IP==0: Check checksums by hardware for incoming IP packets.*/
  #define CHECKSUM_CHECK_IP               0
  /* CHECKSUM_CHECK_UDP==0: Check checksums by hardware for incoming UDP packets.*/
  #define CHECKSUM_CHECK_UDP              0
  /* CHECKSUM_CHECK_TCP==0: Check checksums by hardware for incoming TCP packets.*/
  #define CHECKSUM_CHECK_TCP              0
#else
  /* CHECKSUM_GEN_IP==1: Generate checksums in software for outgoing IP packets.*/
  #define CHECKSUM_GEN_IP                 1
  /* CHECKSUM_GEN_UDP==1: Generate checksums in software for outgoing UDP packets.*/
  #define CHECKSUM_GEN_UDP                1
  /* CHECKSUM_GEN_TCP==1: Generate checksums in software for outgoing TCP packets.*/
  #define CHECKSUM_GEN_TCP                1
  /* CHECKSUM_GEN_ICMP==1: Generate checksums by hardware for outgoing ICMP packets.*/
  #define CHECKSUM_GEN_ICMP               1
  /* CHECKSUM_CHECK_IP==1: Check checksums in software for incoming IP packets.*/
  #define CHECKSUM_CHECK_IP               1
  /* CHECKSUM_CHECK_UDP==1: Check checksums in software for incoming UDP packets.*/
  #define CHECKSUM_CHECK_UDP              1
  /* CHECKSUM_CHECK_TCP==1: Check checksums in software for incoming TCP packets.*/
  #define CHECKSUM_CHECK_TCP              1
#endif

/* ---------- Sequential layer options ---------- */

/* LWIP_NETCONN==1: Enable Netconn API (require to use api_lib.c) */
#define LWIP_NETCONN                    1

/* LWIP_TCPIP_TIMEOUT==1: Enable tcpip_timeout/tcpip_untimeout tod create
 * timers running in tcpip_thread from another thread. */
#define LWIP_TCPIP_TIMEOUT              1

/* ---------- Socket options ---------- */

/* LWIP_SOCKET==1: Enable Socket API (require to use sockets.c) */
#define LWIP_SOCKET                     1

/* ---------- Thread options ---------- */

/* TCPIP_THREAD_NAME: The name assigned to the main tcpip thread. */
#define TCPIP_THREAD_NAME               "TCPIP"

/* TCPIP_THREAD_STACKSIZE: The stack size used by the main tcpip thread.
 * The stack size value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created. */
#define TCPIP_THREAD_STACKSIZE          1024

/* TCPIP_THREAD_PRIO: The priority assigned to the main tcpip thread.
 * The priority value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created. */
#define TCPIP_THREAD_PRIO               (osPriorityHigh)

/* TCPIP_MBOX_SIZE: The mailbox size for the tcpip thread messages
 * The queue size value itself is platform-dependent, but is passed to
 * sys_mbox_new() when tcpip_init is called. */
#define TCPIP_MBOX_SIZE                 16

/* DEFAULT_THREAD_NAME: The name assigned to any other lwIP thread. */
#define DEFAULT_THREAD_NAME             "LWIP"

/* DEFAULT_THREAD_STACKSIZE: The stack size used by any other lwIP thread.
 * The stack size value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created. */
#define DEFAULT_THREAD_STACKSIZE        1024

/* DEFAULT_THREAD_PRIO: The priority assigned to any other lwIP thread.
 * The priority value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created. */
#define DEFAULT_THREAD_PRIO             (osPriorityNormal)

/* DEFAULT_RAW_RECVMBOX_SIZE: The mailbox size for the incoming packets on a
 * NETCONN_RAW. The queue size value itself is platform-dependent, but is passed
 * to sys_mbox_new() when the recvmbox is created. */
#define DEFAULT_RAW_RECVMBOX_SIZE       16

/* DEFAULT_UDP_RECVMBOX_SIZE: The mailbox size for the incoming packets on a
 * NETCONN_UDP. The queue size value itself is platform-dependent, but is passed
 * to sys_mbox_new() when the recvmbox is created. */
#define DEFAULT_UDP_RECVMBOX_SIZE       16

/* DEFAULT_TCP_RECVMBOX_SIZE: The mailbox size for the incoming packets on a
 * NETCONN_TCP. The queue size value itself is platform-dependent, but is passed
 * to sys_mbox_new() when the recvmbox is created. */
#define DEFAULT_TCP_RECVMBOX_SIZE       16

/* DEFAULT_ACCEPTMBOX_SIZE: The mailbox size for the incoming connections.
 * The queue size value itself is platform-dependent, but is passed to
 * sys_mbox_new() when the acceptmbox is created. */
#define DEFAULT_ACCEPTMBOX_SIZE         16

/* ---------- Debugging options ---------- */

#define LWIP_DEBUG                      1

/* LWIP_DBG_MIN_LEVEL: After masking, the value of the debug is
 * compared against this value. If it is smaller, then debugging
 * messages are written. */
#define LWIP_DBG_MIN_LEVEL              LWIP_DBG_LEVEL_ALL

/* LWIP_DBG_TYPES_ON: A mask that can be used to globally enable/disable
 * debug messages of certain types. */
#define LWIP_DBG_TYPES_ON               LWIP_DBG_ON

/* ETHARP_DEBUG: Enable debugging in etharp.c. */
#define ETHARP_DEBUG                    LWIP_DBG_OFF

/* NETIF_DEBUG: Enable debugging in netif.c. */
#define NETIF_DEBUG                     LWIP_DBG_OFF

/* PBUF_DEBUG: Enable debugging in pbuf.c. */
#define PBUF_DEBUG                      LWIP_DBG_OFF

/* API_LIB_DEBUG: Enable debugging in api_lib.c. */
#define API_LIB_DEBUG                   LWIP_DBG_OFF

/* API_MSG_DEBUG: Enable debugging in api_msg.c. */
#define API_MSG_DEBUG                   LWIP_DBG_OFF

/* SOCKETS_DEBUG: Enable debugging in sockets.c. */
#define SOCKETS_DEBUG                   LWIP_DBG_OFF

/* ICMP_DEBUG: Enable debugging in icmp.c. */
#define ICMP_DEBUG                      LWIP_DBG_OFF

/* IGMP_DEBUG: Enable debugging in igmp.c. */
#define IGMP_DEBUG                      LWIP_DBG_OFF

/* INET_DEBUG: Enable debugging in inet.c. */
#define INET_DEBUG                      LWIP_DBG_OFF

/* IP_DEBUG: Enable debugging for IP. */
#define IP_DEBUG                        LWIP_DBG_OFF

/* IP_REASS_DEBUG: Enable debugging in ip_frag.c for both frag & reass. */
#define IP_REASS_DEBUG                  LWIP_DBG_OFF

/* RAW_DEBUG: Enable debugging in raw.c. */
#define RAW_DEBUG                       LWIP_DBG_OFF

/* MEM_DEBUG: Enable debugging in mem.c. */
#define MEM_DEBUG                       LWIP_DBG_OFF

/* MEMP_DEBUG: Enable debugging in memp.c. */
#define MEMP_DEBUG                      LWIP_DBG_OFF

/* SYS_DEBUG: Enable debugging in sys.c. */
#define SYS_DEBUG                       LWIP_DBG_OFF

/* TCP_DEBUG: Enable debugging for TCP. */
#define TCP_DEBUG                       LWIP_DBG_OFF

/* TCP_INPUT_DEBUG: Enable debugging in tcp_in.c for incoming debug. */
#define TCP_INPUT_DEBUG                 LWIP_DBG_OFF

/* TCP_FR_DEBUG: Enable debugging in tcp_in.c for fast retransmit. */
#define TCP_FR_DEBUG                    LWIP_DBG_OFF

/* TCP_RTO_DEBUG: Enable debugging in TCP for retransmit timeout. */
#define TCP_RTO_DEBUG                   LWIP_DBG_OFF

/* TCP_CWND_DEBUG: Enable debugging for TCP congestion window. */
#define TCP_CWND_DEBUG                  LWIP_DBG_OFF

/* TCP_WND_DEBUG: Enable debugging in tcp_in.c for window updating. */
#define TCP_WND_DEBUG                   LWIP_DBG_OFF

/* TCP_OUTPUT_DEBUG: Enable debugging in tcp_out.c output functions. */
#define TCP_OUTPUT_DEBUG                LWIP_DBG_OFF

/* TCP_RST_DEBUG: Enable debugging for TCP with the RST message. */
#define TCP_RST_DEBUG                   LWIP_DBG_OFF

/* TCP_QLEN_DEBUG: Enable debugging for TCP queue lengths. */
#define TCP_QLEN_DEBUG                  LWIP_DBG_OFF

/* UDP_DEBUG: Enable debugging in UDP. */
#define UDP_DEBUG                       LWIP_DBG_OFF

/* TCPIP_DEBUG: Enable debugging in tcpip.c. */
#define TCPIP_DEBUG                     LWIP_DBG_OFF

/* PPP_DEBUG: Enable debugging for PPP. */
#define PPP_DEBUG                       LWIP_DBG_OFF

/* SLIP_DEBUG: Enable debugging in slipif.c. */
#define SLIP_DEBUG                      LWIP_DBG_OFF

/* DHCP_DEBUG: Enable debugging in dhcp.c. */
#define DHCP_DEBUG                      LWIP_DBG_OFF

/* AUTOIP_DEBUG: Enable debugging in autoip.c. */
#define AUTOIP_DEBUG                    LWIP_DBG_OFF

/* SNMP_MSG_DEBUG: Enable debugging for SNMP messages. */
#define SNMP_MSG_DEBUG                  LWIP_DBG_OFF

/* SNMP_MIB_DEBUG: Enable debugging for SNMP MIBs. */
#define SNMP_MIB_DEBUG                  LWIP_DBG_OFF

/* DNS_DEBUG: Enable debugging for DNS. */
#define DNS_DEBUG                       LWIP_DBG_OFF

#endif /* __LWIPOPTS_H__ */

