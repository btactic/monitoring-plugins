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

#include "common.h"
#include "utils.h"

const char *progname = "check_dummy";
const char *revision = "$Revision$";
const char *copyright = "1999-2003";
const char *email = "nagiosplug-devel@lists.sourceforge.net";

void print_help (void);
void print_usage (void);



int
main (int argc, char **argv)
{
	int result;

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	if (argc != 2)
		usage (_("Incorrect number of arguments supplied\n"));
	else if (strcmp (argv[1], "-V") == 0 || strcmp (argv[1], "--version") == 0) {
		print_revision (progname, revision);
		exit (STATE_OK);
	}
	else if (strcmp (argv[1], "-h") == 0 || strcmp (argv[1], "--help") == 0) {
		print_help ();
		exit (STATE_OK);
	}
	else if (!is_integer (argv[1]))
		usage (_("Arguments to check_dummy must be an integer\n"));
	else
		result = atoi (argv[1]);

	switch (result) {
	case STATE_OK:
		printf ("Status is OK\n");
		break;
	case STATE_WARNING:
		printf ("Status is at WARNING level\n");
		break;
	case STATE_CRITICAL:
		printf ("Status is CRITICAL\n");
		break;
	case STATE_UNKNOWN:
		printf ("Status is UNKNOWN\n");
		break;
	default:
		printf ("Status %d is not a supported error state\n", result);
		break;
	}

	return result;
}



void
print_help (void)
{
	print_revision (progname, revision);

	printf (_("Copyright (c) 1999 Ethan Galstad <nagios@nagios.org>\n"));
	printf (_(COPYRIGHT), copyright, email);

	print_usage ();

	printf (_(UT_HELP_VRSN));

	printf (_("\n\
This plugin will simply return the state corresponding to the numeric value\n\
of the <state> argument.\n"));

	printf (_(UT_SUPPORT));
}



void
print_usage (void)
{
	printf (_("Usage: %s <integer state>\n"), progname);
}
