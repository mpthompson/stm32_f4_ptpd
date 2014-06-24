#ifndef __TELNET_SHELL_H__
#define __TELNET_SHELL_H__

#include <stdbool.h>

void telnet_shell_init(void);

void telnet_putc(char c);
void telnet_puts(char *str);
void telnet_printf(const char *fmt, ...);
int telnet_flush(void);
int telnet_getc(void);
bool telnet_gets(char *buff, int len, int tocase, bool echo);

#endif /* __TELNET_SHELL_H__ */
