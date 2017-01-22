/*
 * Copyright (C) 2000 Shoichi Sakane <sakane@tanu.org>, All rights reserved.
 * See the file LICENSE in the top level directory for more details.
 */
#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <err.h>

#include "pub.h"
#include "pub_factory.h"

char *my_name = "heavy_woker";
char *log_level = "info";
int f_debug = 0;

char *prog_name = NULL;

static void
usage()
{
	printf("Usage: %s [OPTIONS]\n", prog_name);
	printf(
"    -B,--broker=url     set the broker URL. (required)\n"
"                        e.g. http://dsa-broker.example.com/conn\n"
"    -D,--dslink=string  set the dslink name of this. (default:%s)\n"
"    -l,--log=level      set the log level.  (info,none,..., default:%s)\n"
"    -d,--debug          increase verbosity.\n"
"    -h,--help           show this help menu.\n"
	, my_name, log_level);

	exit(0);
}

static void
run(char *my_name, char *broker_url, char *log_level)
{
	int argc = 5;
	char **argv;

	if (broker_url == NULL) {
		warnx("ERROR: broker_url must be set.");
		usage();
	}

	if ((argv = calloc(5, sizeof(char *))) == NULL)
		err(1, "calloc(argv)");

	argv[0] = prog_name;
	argv[1] = "--broker";
	argv[2] = broker_url;
	argv[3] = "--log";
	argv[4] = log_level;

	responder_start(argc, argv, my_name);

	warnx("INFO: finished.");

	return;
}

int
main(int argc, char *argv[])
{
	prog_name = 1 + rindex(argv[0], '/');

	const char *shortopts = "D:B:l:dh";
	struct option longopts[] = {
	    {"dslink",    required_argument,  NULL,   'D'},
	    {"broker",    required_argument,  NULL,   'B'},
	    {"log",       required_argument,  NULL,   'l'},
	    {"debug",     no_argument,        NULL,   'd'},
	    {"help",      no_argument,        NULL,   'h'},
	    {NULL,        0,                  NULL,   0}
	};
	char *broker_url = NULL;

	while(1) {
		int ch = getopt_long(argc, argv, shortopts, longopts, NULL);
		if (ch == -1)
			break;
		switch (ch) {
		case 'D':
			my_name = strdup(optarg);
			break;
		case 'B':
			broker_url = strdup(optarg);
			break;
		case 'l':
			log_level = strdup(optarg);
			break;
		case 'd':
			f_debug++;
			break;
		case 'h':
		default:
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc > 0)
		usage();

	run(my_name, broker_url, log_level);

	return 0;
}
