/******************************************************************************

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

******************************************************************************/

const char *progname = "check_smtp";
const char *revision = "$Revision$";
const char *copyright = "2000-2003";
const char *email = "nagiosplug-devel@lists.sourceforge.net";

#include "common.h"
#include "netutils.h"
#include "utils.h"

enum {
	SMTP_PORT	= 25
};
const char *SMTP_EXPECT = "220";
const char *SMTP_HELO = "HELO ";
const char *SMTP_QUIT	= "QUIT\r\n";

int process_arguments (int, char **);
int validate_arguments (void);
void print_help (void);
void print_usage (void);

int server_port = SMTP_PORT;
char *server_address = NULL;
char *server_expect = NULL;
int smtp_use_dummycmd = 1;
char *mail_command;
char *from_arg;
int warning_time = 0;
int check_warning_time = FALSE;
int critical_time = 0;
int check_critical_time = FALSE;
int verbose = 0;






int
main (int argc, char **argv)
{
	int sd;
	double elapsed_time;
	long microsec;
	int result = STATE_UNKNOWN;
	char buffer[MAX_INPUT_BUFFER];
	char *from_str = NULL;
	char *helocmd = NULL;
	struct timeval tv;

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	if (process_arguments (argc, argv) != OK)
		usage (_("Invalid command arguments supplied\n"));

	/* initialize the HELO command with the localhostname */
#ifndef HOST_MAX_BYTES
#define HOST_MAX_BYTES 255
#endif
	helocmd = malloc (HOST_MAX_BYTES);
	gethostname(helocmd, HOST_MAX_BYTES);
	asprintf (&helocmd, "%s%s%s", SMTP_HELO, helocmd, "\r\n");

	/* initialize the MAIL command with optional FROM command  */
	asprintf (&from_str, "%sFROM: %s%s", mail_command, from_arg, "\r\n");

	if (verbose)
		printf ("FROMCMD: %s\n", from_str);
	
	/* initialize alarm signal handling */
	(void) signal (SIGALRM, socket_timeout_alarm_handler);

	/* set socket timeout */
	(void) alarm (socket_timeout);

	/* start timer */
	gettimeofday (&tv, NULL);

	/* try to connect to the host at the given port number */
	result = my_tcp_connect (server_address, server_port, &sd);

	/* we connected, so close connection before exiting */
	if (result == STATE_OK) {

		/* watch for the SMTP connection string and */
		/* return a WARNING status if we couldn't read any data */
		if (recv (sd, buffer, MAX_INPUT_BUFFER - 1, 0) == -1) {
			printf (_("recv() failed\n"));
			result = STATE_WARNING;
		}
		else {
			/* strip the buffer of carriage returns */
			strip (buffer);
			/* make sure we find the response we are looking for */
			if (!strstr (buffer, server_expect)) {
				if (server_port == SMTP_PORT)
					printf (_("Invalid SMTP response received from host\n"));
				else
					printf (_("Invalid SMTP response received from host on port %d\n"),
									server_port);
				result = STATE_WARNING;
			}
		}

		/* send the HELO command */
		send(sd, helocmd, strlen(helocmd), 0);

		/* allow for response to helo command to reach us */
		recv(sd, buffer, MAX_INPUT_BUFFER-1, 0);
				
		/* sendmail will syslog a "NOQUEUE" error if session does not attempt
		 * to do something useful. This can be prevented by giving a command
		 * even if syntax is illegal (MAIL requires a FROM:<...> argument)
		 *
		 * According to rfc821 you can include a null reversepath in the from command
		 * - but a log message is generated on the smtp server.
		 *
		 * You can disable sending mail_command with '--nocommand'
		 * Use the -f option to provide a FROM address
		 */
		if (smtp_use_dummycmd) {

			send(sd, from_str, strlen(from_str), 0);

			/* allow for response to mail_command to reach us */
			recv(sd, buffer, MAX_INPUT_BUFFER-1, 0);

			if (verbose) 
				printf(_("DUMMYCMD: %s\n%s\n"),from_str,buffer);

		} /* smtp_use_dummycmd */

		/* tell the server we're done */
		send (sd, SMTP_QUIT, strlen (SMTP_QUIT), 0);

		/* finally close the connection */
		close (sd);
	}

	/* reset the alarm */
	alarm (0);

	microsec = deltime (tv);
	elapsed_time = (double)microsec / 1.0e6;

	if (check_critical_time && elapsed_time > (double) critical_time)
		result = STATE_CRITICAL;
	else if (check_warning_time && elapsed_time > (double) warning_time)
		result = STATE_WARNING;

	if (verbose)
		printf (_("SMTP %s - %.3f sec. response time, %s|time=%ldus\n"),
		        state_text (result), elapsed_time, buffer, microsec);
	else
		printf (_("SMTP %s - %.3f second response time|time=%ldus\n"),
		        state_text (result), elapsed_time, microsec);

	return result;
}






/* process command-line arguments */
int
process_arguments (int argc, char **argv)
{
	int c;

	int option = 0;
	static struct option longopts[] = {
		{"hostname", required_argument, 0, 'H'},
		{"expect", required_argument, 0, 'e'},
		{"critical", required_argument, 0, 'c'},
		{"warning", required_argument, 0, 'w'},
		{"timeout", required_argument, 0, 't'},
		{"port", required_argument, 0, 'p'},
		{"from", required_argument, 0, 'f'},
		{"command", required_argument, 0, 'C'},
		{"nocommand", required_argument, 0, 'n'},
		{"verbose", no_argument, 0, 'v'},
		{"version", no_argument, 0, 'V'},
		{"use-ipv4", no_argument, 0, '4'},
		{"use-ipv6", no_argument, 0, '6'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};

	if (argc < 2)
		return ERROR;

	for (c = 1; c < argc; c++) {
		if (strcmp ("-to", argv[c]) == 0)
			strcpy (argv[c], "-t");
		else if (strcmp ("-wt", argv[c]) == 0)
			strcpy (argv[c], "-w");
		else if (strcmp ("-ct", argv[c]) == 0)
			strcpy (argv[c], "-c");
	}

	while (1) {
		c = getopt_long (argc, argv, "+hVv46t:p:f:e:c:w:H:C:",
		                 longopts, &option);

		if (c == -1 || c == EOF)
			break;

		switch (c) {
		case 'H':									/* hostname */
			if (is_host (optarg)) {
				server_address = optarg;
			}
			else {
				usage (_("Invalid host name\n"));
			}
			break;
		case 'p':									/* port */
			if (is_intpos (optarg))
				server_port = atoi (optarg);
			else
				usage (_("Server port must be a positive integer\n"));
			break;
		case 'f':									/* from argument */
			from_arg = optarg;
			break;
		case 'e':									/* server expect string on 220  */
			server_expect = optarg;
			break;
		case 'C':									/* server expect string on 220  */
			mail_command = optarg;
			smtp_use_dummycmd = 1;
			break;
		case 'n':									/* server expect string on 220  */
			smtp_use_dummycmd = 0;
			break;
		case 'c':									/* critical time threshold */
			if (is_intnonneg (optarg)) {
				critical_time = atoi (optarg);
				check_critical_time = TRUE;
			}
			else {
				usage (_("Critical time must be a nonnegative integer\n"));
			}
			break;
		case 'w':									/* warning time threshold */
			if (is_intnonneg (optarg)) {
				warning_time = atoi (optarg);
				check_warning_time = TRUE;
			}
			else {
				usage (_("Warning time must be a nonnegative integer\n"));
			}
			break;
		case 'v':									/* verbose */
			verbose++;
			break;
		case 't':									/* timeout */
			if (is_intnonneg (optarg)) {
				socket_timeout = atoi (optarg);
			}
			else {
				usage (_("Time interval must be a nonnegative integer\n"));
			}
			break;
		case '4':
			address_family = AF_INET;
			break;
		case '6':
#ifdef USE_IPV6
			address_family = AF_INET6;
#else
			usage (_("IPv6 support not available\n"));
#endif
			break;
		case 'V':									/* version */
			print_revision (progname, revision);
			exit (STATE_OK);
		case 'h':									/* help */
			print_help ();
			exit (STATE_OK);
		case '?':									/* help */
			usage (_("Invalid argument\n"));
		}
	}

	c = optind;
	if (server_address == NULL) {
		if (argv[c]) {
			if (is_host (argv[c]))
				server_address = argv[c];
			else
				usage (_("Invalid host name"));
		}
		else {
			asprintf (&server_address, "127.0.0.1");
		}
	}

	if (server_expect == NULL)
		server_expect = strdup (SMTP_EXPECT);

	if (mail_command == NULL)
		mail_command = strdup("MAIL ");

	if (from_arg==NULL)
		from_arg = strdup(" ");

	return validate_arguments ();
}





int
validate_arguments (void)
{
	return OK;
}






void
print_help (void)
{
	char *myport;
	asprintf (&myport, "%d", SMTP_PORT);

	print_revision (progname, revision);

	printf (_("Copyright (c) 1999-2001 Ethan Galstad <nagios@nagios.org>\n"));
	printf (_(COPYRIGHT), copyright, email);

	printf(_("\
This plugin will attempt to open an SMTP connection with the host.\n\n"));

	print_usage ();

	printf (_(UT_HELP_VRSN));

	printf (_(UT_HOST_PORT), 'p', myport);

	printf (_(UT_IPv46));

	printf (_("\
 -e, --expect=STRING\n\
   String to expect in first line of server response (default: '%s')\n\
 -n, nocommand\n\
   Suppress SMTP command\n\
 -C, --command=STRING\n\
   SMTP command (default: '%s')\n\
 -f, --from=STRING\n\
   FROM-address to include in MAIL command, required by Exchange 2000\n\
   (default: '%s')\n"), SMTP_EXPECT, mail_command, from_arg);

	printf (_(UT_WARN_CRIT));

	printf (_(UT_TIMEOUT), DEFAULT_SOCKET_TIMEOUT);

	printf (_(UT_VERBOSE));

	printf(_("\n\
Successul connects return STATE_OK, refusals and timeouts return\n\
STATE_CRITICAL, other errors return STATE_UNKNOWN.  Successful\n\
connects, but incorrect reponse messages from the host result in\n\
STATE_WARNING return values.\n"));

	printf (_(UT_SUPPORT));
}





void
print_usage (void)
{
	printf ("\
Usage: %s -H host [-p port] [-e expect] [-C command] [-f from addr]\n\
  [-w warn] [-c crit] [-t timeout] [-n] [-v] [-4|-6]\n", progname);
	printf (_(UT_HLP_VRS), progname, progname);
}

 

