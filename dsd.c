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

struct dsd_property_queue_head *dpqheadp;

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

	return dsd_undefined;
}

int invalid_type(char *typename)
{
	if (!strcasecmp(typename, TYPE_HEX_ADDR_PKG))
		return 0;
	if (!strcasecmp(typename, TYPE_HEX_INTEGER))
		return 0;
	if (!strcasecmp(typename, TYPE_INTEGER))
		return 0;
	if (!strcasecmp(typename, TYPE_STRING))
		return 0;

	return 1;
}

void usage(char *cmd)
{
	fprintf(stderr, "%s, version %s\n", basename(cmd), VERSION);

	fprintf(stderr, "usage: %s <command> [<value> ...]\n", cmd);
	fprintf(stderr, "where <command> is one of:\n");
	fprintf(stderr, "\tadd     => there must be at least two <value>s:\n");
	fprintf(stderr, "\t           the first is the db name, and the\n");
	fprintf(stderr, "\t           remainder are file names containing\n");
	fprintf(stderr, "\t           entries to add to the db\n");
	fprintf(stderr, "\tdelete  => there must be at least two <value>s:\n");
	fprintf(stderr, "\t           the first is the db name, and the\n");
	fprintf(stderr, "\t           remainder are properties to remove\n");
	fprintf(stderr, "\t           from the db\n");
	fprintf(stderr, "\thelp    => this message; no <value> needed\n");
	fprintf(stderr, "\tinit    => create a dsd property db in the\n");
	fprintf(stderr, "\t           directory given by <value>\n");
	fprintf(stderr, "\tlist    => print all known property names from\n");
	fprintf(stderr, "\t           the db name given by <value>\n");
	fprintf(stderr, "\tlookup  => there must be at least two <value>s:\n");
	fprintf(stderr, "\t           the first is the db name, and the\n");
	fprintf(stderr, "\t           remainder are property names to look\n");
	fprintf(stderr, "\t           for in the db; if found, their info\n");
	fprintf(stderr, "\t           will be written to stdout\n");
}

void add_properties(char *dbname)
{
	struct dsd_property *qp;
	struct dsd_property tmp;

	/* whatever is currently in the queue, try to add it to the db */
	if (db_open(dbname)) {
		fprintf(stderr, "? cannot open db: %s\n", dbname);
		exit(1);
	}
	printf("-> Property db: %s\n", dbname);

	for (qp = dpqhead.tqh_first; qp != NULL; qp = qp->entries.tqe_next) {
		if (!db_lookup(qp->property)) {
			printf("? property %s already defined, not added\n",
			       qp->property);
		} else {
			printf("\tadding: %s...", qp->property);
			if (!db_write(qp))
				printf("done.\n");
			else
				printf("failed!\n");
		}
	}

	db_close(dbname);
	printf("-> Finished with db: %s\n", dbname);
}

void set_value(char **field, yaml_event_t *event)
{
	int len;

	len = (event->data.scalar.length < sizeof(int)) ?
			sizeof(int) : event->data.scalar.length;
	*field = malloc(len);
	if (!(*field)) {
		fprintf(stderr, "? no malloc space\n");
		exit(10);
	}
	memset(*field, 0, len);
	strncpy(*field, event->data.scalar.value, event->data.scalar.length);
}

void queue_file(char *filename)
{
	FILE *fp;
	yaml_parser_t parser;
	yaml_event_t event;
	int state;
	int doc_ok;
	struct dsd_property *property = NULL;
	struct dsd_property_value *value = NULL;

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
			state = 0;		/* start state machine */
			doc_ok = 1;
			property = malloc(sizeof(struct dsd_property));
			if (!property) {
				fprintf(stderr, "? cannot malloc property\n");
				exit(1);
			}
			memset(property, 0, sizeof(struct dsd_property));
			break;
		case YAML_DOCUMENT_END_EVENT:
			if (doc_ok)
				TAILQ_INSERT_TAIL(&dpqhead, property, entries);
			else
				event.type = YAML_STREAM_END_EVENT;
			state = 0;
			break;

		case YAML_SEQUENCE_START_EVENT:
			//printf("\t\t-> Sequence Start\n");
			if (state == 0) {
				state = 0;
			} else if (state == 6) {
				state = 7;
				TAILQ_INIT(&property->values);
			} else {
				fprintf(stderr, "? expected a sequence\n");
				event.type = YAML_STREAM_END_EVENT;
				state = 0;
			}
			break;
		case YAML_SEQUENCE_END_EVENT:
			//printf("\t\t-> Sequence End\n");
			state = 0;
			break;
		case YAML_MAPPING_START_EVENT:
			//printf("\t\t-> Mapping Start\n");
			if (state == 0) {
				state = 0;
			} else if (state == 7) {
				state = 8;
				value = malloc(sizeof(struct dsd_property_value));
				if (!value) {
					fprintf(stderr, "? cannot malloc property value\n");
					exit(1);
				}
				memset(value, 0, sizeof(struct dsd_property_value));
			} else {
				fprintf(stderr, "? expected a token: entry\n");
				event.type = YAML_STREAM_END_EVENT;
				state = 0;
			}
			break;
		case YAML_MAPPING_END_EVENT:
			//printf("\t\t-> Mapping End\n");
			break;

		/* data to be used */
		case YAML_SCALAR_EVENT:
			switch (state) {
			case 0:
				if (!strcasecmp(event.data.scalar.value,
				    TOK_PROPERTY)) {
					printf("\tproperty: ");
					state = 1;
				}
				else if (!strcasecmp(event.data.scalar.value,
					 TOK_DESCRIPTION)) {
					printf("\tdescription: |\n");
					state = 2;
				}
				else if (!strcasecmp(event.data.scalar.value,
					 TOK_OWNER)) {
					printf("\towner: ");
					state = 3;
				}
				else if (!strcasecmp(event.data.scalar.value,
					 TOK_EXAMPLE)) {
					printf("\texample: |\n");
					state = 4;
				}
				else if (!strcasecmp(event.data.scalar.value,
					 TOK_TYPE)) {
					printf("\ttype: ");
					state = 5;
				}
				else if (!strcasecmp(event.data.scalar.value,
					 TOK_VALUES)) {
					printf("\tvalues:\n");
					state = 6;
				}
				break;
			case 1:
				printf("%s\n", event.data.scalar.value);
				set_value(&property->property, &event);
				state = 0;
				break;
			case 2:
				printf("\t%s\n", event.data.scalar.value);
				set_value(&property->description, &event);
				state = 0;
				break;
			case 3:
				printf("%s\n", event.data.scalar.value);
				set_value(&property->owner, &event);
				state = 0;
				break;
			case 4:
				printf("\t%s\n", event.data.scalar.value);
				set_value(&property->example, &event);
				state = 0;
				break;
			case 5:
				printf("%s\n", event.data.scalar.value);
				set_value(&property->type, &event);
				if (invalid_type(property->type)) {
					fprintf(stderr, "? invalid type: %s\n",
						property->type);
					exit(1);
				}
				state = 0;
				break;
			/* case 6-7: see sequence start ... */
			case 8:
				if (!strcasecmp(event.data.scalar.value,
				    TOK_TOKEN)) {
					printf("\t\t- token: ");
					state = 9;
				} else {
					state = 0;
				}
				break;
			case 9:
				printf("%s\n", event.data.scalar.value);
				set_value(&value->token, &event);
				state = 10;
				break;
			case 10:
				if (!strcasecmp(event.data.scalar.value,
				    TOK_DESCRIPTION)) {
					printf("\t\t  description: ");
					state = 11;
				}
				break;
			case 11:
				printf("%s\n", event.data.scalar.value);
				set_value(&value->description, &event);
				TAILQ_INSERT_TAIL(&(property->values),
						  value, entries);
				state = 7;
				break;
			default:
				fprintf(stderr, "? internal error, state: %d\n",
				        state);
				exit(1);
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

	/* parameter parsing: 1st arg is always the command */
	if (argc < 2) {
		usage(argv[0]);
		exit(1);
	}

	TAILQ_INIT(&dpqhead);

	command = valid_command(argv[1]);
	switch (command) {
	case dsd_add:
		if (argc < 4) {
			fprintf(stderr, "? a db and file name are required\n");
			exit(1);
		}
		for (ii = 3; ii < argc; ii++)
			queue_file(argv[ii]);
		add_properties(argv[2]);
		break;

	case dsd_delete:
		if (argc < 4) {
			fprintf(stderr, "? a db and property are required\n");
			exit(1);
		}
		if (db_open(argv[2])) {
			fprintf(stderr, "? open failed for %s\n", argv[2]);
			exit(1);
		}
		for (ii = 3; ii < argc; ii++) {
			if (db_lookup(argv[ii]))
				printf("property not found: %s\n", argv[ii]);
			else
				db_delete(argv[ii]);
		}
		db_close(argv[2]);
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
		if (db_open(argv[2])) {
			fprintf(stderr, "? open failed for %s\n", argv[2]);
			exit(1);
		}
		db_list();
		db_close(argv[2]);
		break;

	case dsd_lookup:
		if (argc < 4) {
			fprintf(stderr, "? a db and property are required\n");
			exit(1);
		}
		if (db_open(argv[2])) {
			fprintf(stderr, "? open failed for %s\n", argv[2]);
			exit(1);
		}
		for (ii = 3; ii < argc; ii++) {
			if (db_lookup(argv[ii]))
				printf("property not found: %s\n", argv[ii]);
			else
				db_cat(argv[ii]);
		}
		db_close(argv[2]);
		break;

	default:
		fprintf(stderr, "? invalid command: %s\n", argv[1]);
		usage(argv[0]);
		exit(1);
	}

	while (dpqhead.tqh_first != NULL) {
		qp = dpqhead.tqh_first;
		TAILQ_REMOVE(&dpqhead, dpqhead.tqh_first, entries);
		free(qp);
	}

	return 0;
}
