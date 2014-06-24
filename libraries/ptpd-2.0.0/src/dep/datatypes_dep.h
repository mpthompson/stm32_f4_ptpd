#ifndef DATATYPES_DEP_H_
#define DATATYPES_DEP_H_

// Implementation specific datatypes

// 4-bit enumeration
typedef unsigned char enum4bit_t;

// 8-bit enumeration
typedef unsigned char enum8bit_t;

// 16-bit enumeration
typedef unsigned short enum16bit_t;

// 4-bit  unsigned integer
typedef unsigned char uint4bit_t; 

// 48-bit unsigned integer
typedef struct
{
	unsigned int lsb;
	unsigned short msb;
} uint48bit_t;

// 4-bit data without numerical representation
typedef unsigned char nibble_t;

// 8-bit data without numerical representation
typedef char octet_t; 

// Struct used  to average the offset from master and the one way delay
//
// Exponencial smoothing
//
// alpha = 1/2^s
// y[1] = x[0]
// y[n] = alpha * x[n-1] + (1-alpha) * y[n-1]
//
typedef struct
{
	int32_t   y_prev;
	int32_t   y_sum;
	int16_t   s;
	int16_t   s_prev;
	int32_t n;
} Filter;

// Network  buffer queue
typedef struct
{
	void      *pbuf[PBUF_QUEUE_SIZE];
	int16_t   head;
	int16_t   tail;
	sys_mutex_t mutex;
} BufQueue;

// Struct used  to store network datas
typedef struct
{
	int32_t   multicastAddr;
	int32_t   peerMulticastAddr;
	int32_t   unicastAddr;

	struct udp_pcb    *eventPcb;
	struct udp_pcb    *generalPcb;

	BufQueue    eventQ;
	BufQueue    generalQ;
} NetPath;

// Define compiler specific symbols
#if defined   ( __CC_ARM   )
typedef long ssize_t;
#elif defined ( __ICCARM__ )
typedef long ssize_t;
#elif defined (  __GNUC__  )

#elif defined   (  __TASKING__  )
typedef long ssize_t;
#endif

#endif /* DATATYPES_DEP_H_*/
