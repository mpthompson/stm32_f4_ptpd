#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "string.h"
#include "shell.h"
#include "telnet.h"

#define TELNET_THREAD_PRIO    ( osPriorityNormal )

// Telnet states
#define STATE_NORMAL 0
#define STATE_IAC    1
#define STATE_OPT    2
#define STATE_SB     3
#define STATE_OPTDAT 4
#define STATE_SE     5

// Telnet special characters
#define TELNET_SE    240   // End of subnegotiation parameters
#define TELNET_NOP   241   // No operation
#define TELNET_MARK  242   // Data mark
#define TELNET_BRK   243   // Break
#define TELNET_IP    244   // Interrupt process
#define TELNET_AO    245   // Abort output
#define TELNET_AYT   246   // Are you there
#define TELNET_EC    247   // Erase character
#define TELNET_EL    248   // Erase line
#define TELNET_GA    249   // Go ahead
#define TELNET_SB    250   // Start of subnegotiation parameters
#define TELNET_WILL  251   // Will option code
#define TELNET_WONT  252   // Won't option code
#define TELNET_DO    253   // Do option code
#define TELNET_DONT  254   // Don't option code
#define TELNET_IAC   255   // Interpret as command

// Telnet options
#define TELOPT_TRANSMIT_BINARY      0  // Binary Transmission (RFC856)
#define TELOPT_ECHO                 1  // Echo (RFC857)
#define TELOPT_SUPPRESS_GO_AHEAD    3  // Suppress Go Ahead (RFC858)
#define TELOPT_STATUS               5  // Status (RFC859)
#define TELOPT_TIMING_MARK          6  // Timing Mark (RFC860)
#define TELOPT_NAOCRD              10  // Output Carriage-Return Disposition (RFC652)
#define TELOPT_NAOHTS              11  // Output Horizontal Tab Stops (RFC653)
#define TELOPT_NAOHTD              12  // Output Horizontal Tab Stop Disposition (RFC654)
#define TELOPT_NAOFFD              13  // Output Formfeed Disposition (RFC655)
#define TELOPT_NAOVTS              14  // Output Vertical Tabstops (RFC656)
#define TELOPT_NAOVTD              15  // Output Vertical Tab Disposition (RFC657)
#define TELOPT_NAOLFD              16  // Output Linefeed Disposition (RFC658)
#define TELOPT_EXTEND_ASCII        17  // Extended ASCII (RFC698)
#define TELOPT_TERMINAL_TYPE       24  // Terminal Type (RFC1091)
#define TELOPT_NAWS                31  // Negotiate About Window Size (RFC1073)
#define TELOPT_TERMINAL_SPEED      32  // Terminal Speed (RFC1079)
#define TELOPT_TOGGLE_FLOW_CONTROL 33  // Remote Flow Control (RFC1372)
#define TELOPT_LINEMODE            34  // Linemode (RFC1184)
#define TELOPT_AUTHENTICATION      37  // Authentication (RFC1416)

#define TERM_UNKNOWN 0
#define TERM_CONSOLE 1
#define TERM_VT100   2

#define TELNET_BUF_SIZE 512

struct term
{
  int type;
  int cols;
  int lines;
};

struct telbuf
{
  unsigned char data[TELNET_BUF_SIZE];
  unsigned char *start;
  unsigned char *end;
};

struct termstate
{
  int sock;
  int state;
  int code;
  unsigned char optdata[256];
  int optlen;
  struct term term;
  struct telbuf bi;
  struct telbuf bo;
};

static struct termstate tstate;
struct termstate *ts = &tstate;

static void telnet_sendopt(struct termstate *ts, int code, int option)
{
  unsigned char buf[3];
  buf[0] = TELNET_IAC;
  buf[1] = (unsigned char) code;
  buf[2] = (unsigned char) option;
  send(ts->sock, buf, 3, 0);
}

static void telnet_parseopt(struct termstate *ts, int code, int option)
{
  switch (option)
  {
    case TELOPT_ECHO:
      break;
    case TELOPT_SUPPRESS_GO_AHEAD:
      if (code == TELNET_WILL || code == TELNET_WONT)
        telnet_sendopt(ts, TELNET_DO, option);
      else
        telnet_sendopt(ts, TELNET_WILL, option);
      break;
    case TELOPT_TERMINAL_TYPE:
    case TELOPT_NAWS:
    case TELOPT_TERMINAL_SPEED:
      telnet_sendopt(ts, TELNET_DO, option);
      break;
    default:
      if ((code == TELNET_WILL) || (code == TELNET_WONT))
        telnet_sendopt(ts, TELNET_DONT, option);
      else
        telnet_sendopt(ts, TELNET_WONT, option);
      break;
  }
}

static void telnet_parseoptdat(struct termstate *ts, int option, unsigned char *data, int len)
{
  switch (option)
  {
    case TELOPT_NAWS:
      if (len == 4)
      {
        int cols = ntohs(*(unsigned short *) data);
        int lines = ntohs(*(unsigned short *) (data + 2));
        if (cols) ts->term.cols = cols;
        if (lines) ts->term.lines = lines;
      }
      break;
    case TELOPT_TERMINAL_SPEED:
      break;
    case TELOPT_TERMINAL_TYPE:
      break;
  }
}

static void telnet_process(struct termstate *ts)
{
  unsigned char *p = ts->bi.start;
  unsigned char *q = p;
  while (p < ts->bi.end)
  {
    int c = *p++;
    switch (ts->state) 
    {
      case STATE_NORMAL:
        if (c == TELNET_IAC)
          ts->state = STATE_IAC;
        else
          *q++ = c;
        break;
      case STATE_IAC:
        switch (c) 
        {
          case TELNET_IAC:
            *q++ = c;
            ts->state = STATE_NORMAL;
            break;
          case TELNET_WILL:
          case TELNET_WONT:
          case TELNET_DO:
          case TELNET_DONT:
            ts->code = c;
            ts->state = STATE_OPT;
            break;
          case TELNET_SB:
            ts->state = STATE_SB;
            break;
          default:
            ts->state = STATE_NORMAL;
        }
        break;
      case STATE_OPT:
        telnet_parseopt(ts, ts->code, c);
        ts->state = STATE_NORMAL;
        break;
      case STATE_SB:
        ts->code = c;
        ts->optlen = 0;
        ts->state = STATE_OPTDAT;
        break;
      case STATE_OPTDAT:
        if (c == TELNET_IAC)
          ts->state = STATE_SE;
        else if (ts->optlen < sizeof(ts->optdata))
          ts->optdata[ts->optlen++] = c;
        break;
      case STATE_SE:
        if (c == TELNET_SE)
          telnet_parseoptdat(ts, ts->code, ts->optdata, ts->optlen);
        ts->state = STATE_NORMAL;
        break;
    } 
  }
  ts->bi.end = q;
}

static int telnet_write(char *str, int size, bool flush)
{
  int i;
	int n;
	int len;
  fd_set data_write;
  struct timeval tv;
  unsigned char *ptr = (unsigned char *) str;

	n = 0;
  do
  {
    len = TELNET_BUF_SIZE - (int)(ts->bo.start - ts->bo.data);
    if (size < len) len = size;
    if (len >= 0)
    {
      memcpy(ts->bo.start, str, len);
      size -= len;
      ts->bo.start += len;
      str += len;  
    }
    if (((int)(ts->bo.start - ts->bo.data) >= TELNET_BUF_SIZE) || flush)
    {
      len = n = 0;
      ptr = ts->bo.data;
      do
      {
        FD_ZERO(&data_write);
        FD_SET(ts->sock, &data_write);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        i = select(FD_SETSIZE, NULL, &data_write, NULL, &tv);
        if ((i > 0) && (FD_ISSET(ts->sock, &data_write) != 0))
        {
          n = send(ts->sock, ptr, (int) (ts->bo.start - ts->bo.data) - len, 0);
          if (n >= 0)
          {
            ptr += n;
            len += n;
          }
        }
      }
      while ((n >= 0) && (len < (int) (ts->bo.start - ts->bo.data)) && (i > 0));
      ts->bo.start = ts->bo.data;
    }
  }
  while (size > 0);

  return n < 0 ? n : len;
}

static void telnet_shell(int s) 
{
  int off;

	// TCP_NODELAY is used to disable the Nagle buffering algorithm. It 
	// should only be set for applications such as telnet that send frequent
  // small bursts of information without getting an immediate response,
  // where timely delivery of data is required.
	off = 0; setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char *) &off, sizeof(off));

	// Initialize terminal state.
	memset(ts, 0, sizeof(struct termstate));
	ts->sock = s;
	ts->state = STATE_NORMAL;
	ts->term.type = TERM_VT100;
	ts->term.cols = 80;
	ts->term.lines = 25;

	// Send initial option that we'll echo on this side.
	telnet_sendopt(ts, TELNET_WILL, TELOPT_ECHO);

	// Reset the output buffer.
	ts->bo.start = ts->bo.data;

	// Run the shell.
	shell_process();

	// Close the socket.
	if (ts->sock >= 0)
	{
		close(ts->sock);
		ts->sock = -1;
	}
}

static void telnet_shell_thread(void *arg)
{
  int sock, newconn, size;
  struct sockaddr_in address, remotehost;

	// Create a TCP socket.
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
  {
    printf("TELNET: cannot create socket");
    return;
  }
  
  // Bind to port 23 at any interface.
  address.sin_family = AF_INET;
  address.sin_port = htons(23);
  address.sin_addr.s_addr = INADDR_ANY;

  if (bind(sock, (struct sockaddr *)&address, sizeof (address)) < 0)
  {
    printf("TELNET: cannot bind socket");
    return;
  }
  
  // Listen for incoming connections (TCP listen backlog = 1).
  listen(sock, 1);
  
  size = sizeof(remotehost);
  
  for (;;) 
  {
    newconn = accept(sock, (struct sockaddr *)&remotehost, (socklen_t *)&size);
    if (newconn > 0) telnet_shell(newconn);
  }
}

void telnet_shell_init(void)
{
  sys_thread_new("TELNET", telnet_shell_thread, NULL, DEFAULT_THREAD_STACKSIZE, TELNET_THREAD_PRIO);
}

void telnet_putc(char c)
{
	if (c == '\n') telnet_putc('\r');
	if (ts->sock > 0) telnet_write(&c, 1, (c == '\n') ? true : false);
}

void telnet_puts(char *str)
{
	// Send each character in the string.
	while (*str) telnet_putc(*(str++));
}

void telnet_printf(const char *fmt, ...)
{
	char c;
	char d;
	char *p;
	char s[16];
	unsigned int f;
	unsigned int r;
	unsigned int i;
	unsigned int j;
	unsigned int w;
	unsigned long v;
	__va_list arp;

	va_start (arp, fmt);
	for (;;)
	{
		// Get a character.
		c = *(fmt++);
		// End of format?
		if (!c) break;
		// Pass through if it not a % sequence.
		if (c != '%')
		{
			telnet_putc(c);
			continue;
		}
		f = 0;
		// Get first character of % sequence.
		c = *(fmt++);
		// Flag: '0' padded.
		if (c == '0')
		{
			f = 1;
			c = *(fmt++);
		}
		else
		{
			// Flag: left justified.
			if (c == '-')
			{
				f = 2;
				c = *(fmt++);
			}
		}
		for (w = 0; c >= '0' && c <= '9'; c = *(fmt++))	/* Minimum width */
			w = w * 10 + c - '0';
		// Prefix: Size is long int.
		if (c == 'l' || c == 'L')
		{
			f |= 4;
			c = *(fmt++);
		}
		// End of format.
		if (!c) break;
		d = c;
		if (d >= 'a') d -= 0x20;
		// Type is...
		switch (d) {
		// String.
		case 'S' :
			p = va_arg(arp, char*);
			for (j = 0; p[j]; j++);
			while (!(f & 2) && j++ < w) telnet_putc(' ');
			while (*p) telnet_putc(*(p++));
			while (j++ < w) telnet_putc(' ');
			continue;
		// Character.
		case 'C' :
			telnet_putc((char) va_arg(arp, int));
			continue;
		// Binary.
		case 'B' :
			r = 2;
			break;
		// Octal.
		case 'O' :
			r = 8;
			break;
		// Signed decimal.
		case 'D' :
		// Unsigned decimal.
		case 'U' :
			r = 10;
			break;
		// Hexadecimal.
		case 'X' :
			r = 16;
			break;
		// Unknown type (passthrough).
		default:
			telnet_putc(c);
			continue;
		}
		// Get an argument and put it in numeral.
		v = (f & 4) ? va_arg(arp, long) : ((d == 'D') ? (long) va_arg(arp, int) : (long) va_arg(arp, unsigned int));
		if (d == 'D' && (v & 0x80000000)) {
			v = 0 - v;
			f |= 8;
		}
		i = 0;
		do {
			d = (char)(v % r); v /= r;
			if (d > 9) d += (c == 'x') ? 0x27 : 0x07;
			s[i++] = d + '0';
		} while (v && i < sizeof(s));
		if (f & 8) s[i++] = '-';
		j = i; d = (f & 1) ? '0' : ' ';
		while (!(f & 2) && j++ < w) telnet_putc(d);
		do telnet_putc(s[--i]); while(i);
		while (j++ < w) telnet_putc(' ');
	}
	va_end(arp);
}

// Flush the telnet output buffers.
int telnet_flush(void)
{
  int i;
	int n;
	int len;
  fd_set data_write;
  struct timeval tv;
  unsigned char *ptr;

	len = n = 0;

	// Is there data to flush?
	if (ts->bo.start > ts->bo.data)
	{
		ptr = ts->bo.data;
		do
		{
			FD_ZERO(&data_write);
			FD_SET(ts->sock, &data_write);
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			i = select(FD_SETSIZE, NULL, &data_write, NULL, &tv);
			if ((i > 0) && (FD_ISSET(ts->sock, &data_write) != 0))
			{
				n = send(ts->sock, ptr, (int) (ts->bo.start - ts->bo.data) - len, 0);
				if (n >= 0)
				{
					ptr += n;
					len += n;
				}
			}
		}
		while ((n >= 0) && (len < (int) (ts->bo.start - ts->bo.data)) && (i > 0));
		ts->bo.start = ts->bo.data;
	}

  return n < 0 ? n : len;
}

// Get the next character from the buffer.
int telnet_getc(void)
{
  int i;
	int n;
	int ch;
  fd_set fdsr;
  struct timeval tv_timeout;

	// Do we need to fill the input buffer with new data?
	while (ts->bi.start == ts->bi.end)
	{
		FD_ZERO(&fdsr);
		FD_SET(ts->sock, &fdsr);
		tv_timeout.tv_sec = 1;
		tv_timeout.tv_usec = 0;

		// Wait for input activity on the socket.
		i = select(FD_SETSIZE, &fdsr, NULL, NULL, &tv_timeout);

		// Was a error returned?
		if (i < 0) break;

		// Do we have data to read?
    if ((i > 0) && FD_ISSET(ts->sock, &fdsr))
    {
			// Read data from user.
			n = recv(ts->sock, ts->bi.data, sizeof(ts->bi.data), 0);

			// Was a error returned?
			if (n < 0) break;
	
			// End of stream?
			if (n == 0) break;

			// Set the input buffer.
			ts->bi.start = ts->bi.data;
			ts->bi.end = ts->bi.data + n;        

			// Process input for telnet options.
			telnet_process(ts);
		}

		telnet_flush();
	}

	// If no data then either error or end of stream.
	if (ts->bi.start == ts->bi.end) return -1;

	// Get the next character in the buffer.
	ch = (unsigned char) *(ts->bi.start++);

	return ch;
}

bool telnet_gets(char *buff, int len, int tocase, bool echo)
{
	int c;
	int i;

	i = 0;
	for (;;)
	{
		// Get a char from the incoming stream.
		c = telnet_getc();

		// End of stream?
		if (c < 0) return false;

		// Convert to upper/lower case?
		if (tocase > 0) c = toupper(c);
		if (tocase < 0) c = tolower(c);

		// End of line?
		if (c == '\r') break;

		// Back space?
		if (c == '\b' && i)
		{
			i--;
			if (echo) { telnet_putc(c); telnet_putc(' '); telnet_putc(c); }
		}

		// Visible chars.
		if ((c >= ' ') && (i < (len - 1)))
		{
			buff[i++] = c;
			if (echo) telnet_putc(c);
		}

		telnet_flush();
	}

	// Null terminate.
	buff[i] = 0;
	if (echo)
	{
		telnet_puts("\r\n");
		telnet_flush();
	}

	return true;
}

