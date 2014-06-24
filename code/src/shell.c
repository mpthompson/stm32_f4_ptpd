#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "cmsis_os.h"
#include "ptpd.h"
#include "shell.h"
#include "telnet.h"

typedef bool (*shell_func)(int argc, char **argv);

struct shell_command 
{
	char name[16];
	shell_func funcptr;
};

static bool shell_exit(int argc, char **argv);
static bool shell_help(int argc, char **argv);
static bool shell_date(int argc, char **argv);
static bool shell_ptpd(int argc, char **argv);

// Must be sorted in ascending order.
const struct shell_command commands[] = 
{
	{"DATE", shell_date},
	{"EXIT", shell_exit},
	{"HELP", shell_help},
	{"PTPD", shell_ptpd},
};

static bool shell_exit(int argc, char **argv)
{
	// Exit the shell interpreter.
	return false;
}

static bool shell_help(int argc, char **argv)
{
	uint32_t i;

	// Loop over each shell command.
	for (i = 0; i < sizeof(commands) / sizeof(struct shell_command); ++i)
	{
		telnet_printf("%s\n", commands[i].name);
	}

	return true;
}

static bool shell_date(int argc, char **argv)
{
	char buffer[32];
	time_t seconds1900;
	struct ptptime_t ptptime;

	// Get the ethernet time values.
	ETH_PTPTime_GetTime(&ptptime);

	// Get the seconds since 1900.
	seconds1900 = (time_t) ptptime.tv_sec;

	// Format into a string.
	strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S UTC %Y\n", localtime(&seconds1900));

	// Print the string.
	telnet_puts(buffer);

	return true;
}

static bool shell_ptpd(int argc, char **argv)
{
	char sign;
	const char *s;
	unsigned char *uuid;
	extern PtpClock ptpClock;

	uuid = (unsigned char*) ptpClock.parentDS.parentPortIdentity.clockIdentity;

	/* Master clock UUID */
	telnet_printf("master id: %02x%02x%02x%02x%02x%02x%02x%02x\n",
					uuid[0], uuid[1],
					uuid[2], uuid[3],
					uuid[4], uuid[5],
					uuid[6], uuid[7]);

	switch (ptpClock.portDS.portState)
	{
		case PTP_INITIALIZING:  s = "init";  break;
		case PTP_FAULTY:        s = "faulty";   break;
		case PTP_LISTENING:     s = "listening";  break;
		case PTP_PASSIVE:       s = "passive";  break;
		case PTP_UNCALIBRATED:  s = "uncalibrated";  break;
		case PTP_SLAVE:         s = "slave";   break;
		case PTP_PRE_MASTER:    s = "pre master";  break;
		case PTP_MASTER:        s = "master";   break;
		case PTP_DISABLED:      s = "disabled";  break;
		default:                s = "?";     break;
	}

	/* State of the PTP */
	telnet_printf("state: %s\n", s);

	/* One way delay */
	switch (ptpClock.portDS.delayMechanism)
	{
		case E2E:
			telnet_puts("mode: end to end\n");
			telnet_printf("path delay: %d nsec\n", ptpClock.currentDS.meanPathDelay.nanoseconds);
			break;
		case P2P:
			telnet_puts("mode: peer to peer\n");
			telnet_printf("path delay: %d nsec\n", ptpClock.portDS.peerMeanPathDelay.nanoseconds);
			break;
		default:
			telnet_puts("mode: unknown\n");
			telnet_printf("path delay: unknown\n");
			/* none */
			break;
	}

	/* Offset from master */
	if (ptpClock.currentDS.offsetFromMaster.seconds)
	{
		telnet_printf("offset: %d sec\n", ptpClock.currentDS.offsetFromMaster.seconds);
	}
	else
	{
		telnet_printf("offset: %d nsec\n", ptpClock.currentDS.offsetFromMaster.nanoseconds);
	}

	/* Observed drift from master */
	sign = ' ';
	if (ptpClock.observedDrift > 0) sign = '+';
	if (ptpClock.observedDrift < 0) sign = '-';

	telnet_printf("drift: %c%d.%03d ppm\n", sign, abs(ptpClock.observedDrift / 1000), abs(ptpClock.observedDrift % 1000));

	return true;
}

// Parse out the next non-space word from a string.
// str		Pointer to pointer to the string
// word		Pointer to pointer of next word.
// Returns 0:Failed, 1:Successful
static int shell_parse(char **str, char **word)
{
  // Skip leading spaces.
	while (**str && isspace(**str)) (*str)++;

  // Set the word.
  *word = *str;

  // Skip non-space characters.
	while (**str && !isspace(**str)) (*str)++;

  // Null terminate the word.
  if (**str) *(*str)++ = 0;

  return (*str != *word) ? 1 : 0;
}

// Attempt to execute the shell command.
static bool shell_command(char *cmdline)
{
	int i;
	char *argv[8];
	int argc = 0;
	bool rv = true;
	struct shell_command *command;

	// Parse the command and any arguments into an array.
	for (i = 0; i < (sizeof(argv) / sizeof(char *)); ++i)
	{
		shell_parse(&cmdline, &argv[i]);
		if (*argv[i] != 0) ++argc;
	}

	// Search for a matching command.
	command = (struct shell_command *) bsearch(argv[0], commands, sizeof(commands) / sizeof(struct shell_command), sizeof(struct shell_command), (int(*)	(const void*, const void*)) strcasecmp);

	// Call the command if found.
	if (command) rv = (command->funcptr)(argc, argv);

	return rv;
}

// Runs the shell by printing the prompt and processing each shell command typed in.
void shell_process() 
{
	char cmdbuf[64];

	// Tell the user the shell is starting.
	telnet_printf("Starting Shell...\n");

	// Send a prompt.
	telnet_puts("> ");
	telnet_flush();

	// Get a string.
  while (telnet_gets(cmdbuf, sizeof(cmdbuf), 0, true))
  {
		// Process the line as a command.
		if (!shell_command(cmdbuf)) break;

		// Send a prompt.
		telnet_puts("> ");
		telnet_flush();
	}
}

