/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Al Stone <ahs3@redhat.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 ******************************************************************************
 */

#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <yaml.h>

#include <sys/queue.h>

#include "dsd.h"
#include "db.h"
#include "parse.h"
#include "version.h"

struct dsd_property_queue_head *dpqheadp;
struct dsd_device_queue_head *ddqheadp;

enum dsd_command valid_command(char *cmd)
{
	if (!strcasecmp(cmd, CMD_ADD))
		return dsd_add;
	if (!strcasecmp(cmd, CMD_DELETE))
		return dsd_delete;
	if (!strcasecmp(cmd, CMD_HELP))
		return dsd_help;
	if (!strcasecmp(cmd, CMD_INIT))
		return dsd_init;
	if (!strcasecmp(cmd, CMD_LIST))
		return dsd_list;
	if (!strcasecmp(cmd, CMD_LOOKUP))
		return dsd_lookup;
	if (!strcasecmp(cmd, CMD_VERIFY))
		return dsd_verify;

	return dsd_undefined;
}

void usage(char *cmd)
{
	fprintf(stderr, "%s, version %s\n", basename(cmd), VERSION);

	fprintf(stderr, "usage: %s <command> <dbname> [<value> ...]\n", cmd);
	fprintf(stderr, "where <command> is one of:\n");
	fprintf(stderr, "\tadd     => one or more <value>s are required,\n");
	fprintf(stderr, "\t           each a file name containing entries\n");
	fprintf(stderr, "\t           to add to <dbname>\n");
	fprintf(stderr, "\tdelete  => one or more <value>s are required,\n");
	fprintf(stderr, "\t           each a property or device to remove\n");
	fprintf(stderr, "\t           from <dbname>\n");
	fprintf(stderr, "\thelp    => this message; no <value> needed\n");
	fprintf(stderr, "\tinit    => create a dsd property db in the\n");
	fprintf(stderr, "\t           directory given by <dbname>\n");
	fprintf(stderr, "\tlist [devs | props | all]\n");
	fprintf(stderr, "\t        => if \"devs\", print all known devices\n");
	fprintf(stderr, "\t           in <dbname>, if \"props\" print all\n");
	fprintf(stderr, "\t           known properties, and if \"all\" or\n");
	fprintf(stderr, "\t           not given, do both\n");
	fprintf(stderr, "\tlookup  => one or more <value>s are required,\n");
	fprintf(stderr, "\t           each a device or property name to\n");
	fprintf(stderr, "\t           to look for in <dbname>; if found,\n");
	fprintf(stderr, "\t           their info will be written to stdout\n");
	fprintf(stderr, "\tverify  => run all content checks on a db\n");
}

void add_devices(char *dbname)
{
	struct dsd_device *dp;

	/* whatever is currently in the queue, try to add it to the db */
	printf("-> Device db: %s\n", dbname);

	for (dp = ddqhead.tqh_first; dp != NULL; dp = dp->entries.tqe_next) {
		if (!db_dev_lookup(dp->device)) {
			printf("? device %s already defined, not added\n",
			       dp->device);
		} else {
			printf("\tadding: %s...", dp->device);
			if (!db_dev_write(dp))
				printf("done.\n");
			else
				printf("failed!\n");
		}
	}

	printf("-> Finished with db: %s\n", dbname);
}

void add_properties(char *dbname)
{
	struct dsd_property *qp;

	/* whatever is currently in the queue, try to add it to the db */
	printf("-> Property db: %s\n", dbname);

	for (qp = dpqhead.tqh_first; qp != NULL; qp = qp->entries.tqe_next) {
		if (!db_prop_lookup(qp->property)) {
			printf("? property %s already defined, not added\n",
			       qp->property);
		} else {
			printf("\tadding: %s...", qp->property);
			if (!db_prop_write(qp))
				printf("done.\n");
			else
				printf("failed!\n");
		}
	}

	printf("-> Finished with db: %s\n", dbname);
}

void fill_buf(FILE *fp, size_t begin, size_t end, char *buf)
{
	size_t len;

	len = end - begin;
	memset(buf, 0, len + 1);
	if (!fseek(fp, begin, SEEK_SET)) {
		fread(buf, 1, len, fp);
		fseek(fp, end, SEEK_SET);
	} else {
		fprintf(stderr, "? cannot fill buf (%ld, %ld)\n", begin, end);
		exit(1);
	}
}

void queue_file(char *filename)
{
	FILE *fp;
	yaml_parser_t parser;
	yaml_event_t event;
	size_t begin, end;
	enum dsd_doc_type doc;
	char *buf;
	struct dsd_device *device;
	struct dsd_property *property;

	fp = fopen(filename, "r");
	if (!fp) {
		fprintf(stderr, "? cannot open yaml file: %s\n", filename);
		exit(1);
	}

	/* initialize the parser */
	if (!yaml_parser_initialize(&parser)) {
		fprintf(stderr, "? cannot initialize parser\n");
		exit(2);
	}

	/* tell the parser what file to read */
	yaml_parser_set_input_file(&parser, fp);

	/* do something with the content of the file */
	do {
		if (!yaml_parser_parse(&parser, &event)) {
			fprintf(stderr, "? parser error: %d\n", parser.error);
			exit(EXIT_FAILURE);
		}

		switch(event.type) {
		case YAML_NO_EVENT:
			break;

		/* stream delimiters */
		case YAML_STREAM_START_EVENT:
			printf("-> Parsing file: %s\n", filename);
			break;
		case YAML_STREAM_END_EVENT:
			printf("-> Finished file: %s\n", filename);
			break;

		/* block delimiters */
		case YAML_DOCUMENT_START_EVENT:
			begin = event.start_mark.index;
			doc = DUNNO;
			break;
		case YAML_DOCUMENT_END_EVENT:
			end = event.start_mark.index;
			if (end - begin < 3) { /* at least '---' */
				fprintf(stderr, "? bogus doc at %ld\n", begin);
				exit(1);
			}
			buf = malloc(end - begin + 1);
			if (!buf) {
				fprintf(stderr, "? cannot malloc new doc\n");
				exit(1);
			}
			fill_buf(fp, begin, end, buf);
			if (doc == DEVDOC) {
				device = parse_dev_doc(buf, 1);
				if (device)
					TAILQ_INSERT_TAIL(&ddqhead, device,
							  entries);
			} else if (doc == PROPDOC) {
				property = parse_prop_doc(buf, 1);
				if (property)
					TAILQ_INSERT_TAIL(&dpqhead, property,
							  entries);
			} else {
				fprintf(stderr, "? unknown doc type\n");
				event.type = YAML_STREAM_END_EVENT;
			}
			doc = DUNNO;
			break;

		/* data to be used */
		case YAML_SCALAR_EVENT:
			if (doc == DUNNO) {
				if (!strcasecmp(event.data.scalar.value,
			    	    TOK_PROPERTY)) {
					/* start a property document */
					doc = PROPDOC;
				}
				else if (!strcasecmp(event.data.scalar.value,
			    	    TOK_DEVICE)) {
					/* start a device document */
					doc = DEVDOC;
				}
			}
			break;
		}

		if (event.type != YAML_STREAM_END_EVENT)
			yaml_event_delete(&event);

	} while (event.type != YAML_STREAM_END_EVENT);

	/* shutdown nicely */
	yaml_parser_delete(&parser);
	fclose(fp);
}

int main(int argc, char *argv[])
{
	enum dsd_command command;
	int ii;
	struct dsd_property *qp;
	struct dsd_device *dp;

	/* parameter parsing: 1st arg is always the command */
	if (argc < 2) {
		usage(argv[0]);
		exit(1);
	}

	TAILQ_INIT(&dpqhead);
	TAILQ_INIT(&ddqhead);

	command = valid_command(argv[1]);
	if (strcasecmp(argv[1], CMD_INIT) && db_open(argv[2])) {
		fprintf(stderr, "? open failed for %s\n", argv[2]);
		exit(1);
	}
	switch (command) {
	case dsd_add:
		if (argc < 4) {
			fprintf(stderr, "? a db and file name are required\n");
			exit(1);
		}
		for (ii = 3; ii < argc; ii++)
			queue_file(argv[ii]);
		add_properties(argv[2]);
		add_devices(argv[2]);
		db_verify(argv[2]);
		break;

	case dsd_delete:
		if (argc < 4) {
			fprintf(stderr, "? a db and name are required\n");
			exit(1);
		}
		for (ii = 3; ii < argc; ii++) {
			if (db_lookup(argv[ii]))
				printf("entry not found: %s\n", argv[ii]);
			else
				db_delete(argv[ii]);
		}
		db_verify(argv[2]);
		break;

	case dsd_help:
		usage(argv[0]);
		exit(0);
		break;

	case dsd_init:
		if (argc < 3) {
			fprintf(stderr, "? a directory name is required\n");
			exit(1);
		}
		if (db_init(argv[2])) {
			fprintf(stderr, "? init failed for %s\n", argv[2]);
			exit(1);
		}
		break;

	case dsd_list:
		if (argc < 3) {
			fprintf(stderr, "? a db is required\n");
			exit(1);
		}
		if (argc >= 3) {
			if (argc == 3)
				db_list();
			else if (strcasecmp(argv[3], SUBCMD_DEVS) == 0)
				db_dev_list();
			else if (strcasecmp(argv[3], SUBCMD_PROPS) == 0)
				db_prop_list();
			else {
				fprintf(stderr, "? ignoring extra values\n");
				db_list();
			}
		}
		break;

	case dsd_lookup:
		if (argc < 4) {
			fprintf(stderr, "? a db and property or device are required\n");
			exit(1);
		}
		for (ii = 3; ii < argc; ii++) {
			if (db_lookup(argv[ii]))
				printf("entry not found: %s\n", argv[ii]);
			else
				db_cat(argv[ii]);
		}
		break;

	case dsd_verify:
		if (argc < 3) {
			fprintf(stderr, "? a db is required\n");
			exit(1);
		}
		db_verify(argv[2]);
		break;

	default:
		fprintf(stderr, "? invalid command: %s\n", argv[1]);
		usage(argv[0]);
		exit(1);
	}

	db_close(argv[2]);
	while (dpqhead.tqh_first != NULL) {
		qp = dpqhead.tqh_first;
		TAILQ_REMOVE(&dpqhead, dpqhead.tqh_first, entries);
		free(qp);
	}
	while (ddqhead.tqh_first != NULL) {
		dp = ddqhead.tqh_first;
		TAILQ_REMOVE(&ddqhead, ddqhead.tqh_first, entries);
		free(dp);
	}

	return 0;
}
