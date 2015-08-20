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

#include "db.h"
#include "dsd.h"
#include "parse.h"

int find_dev(char *name)
{
	struct dsd_device *dp;

	for (dp = ddqhead.tqh_first; dp != NULL; dp = dp->entries.tqe_next) {
		if (!strcasecmp(dp->device, name))
			return 0;
	}
	return 1;
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

void set_value(char **field, yaml_event_t *event)
{
	int len;

	len = (event->data.scalar.length < sizeof(int)) ?
			sizeof(int) : event->data.scalar.length + 1;
	*field = malloc(len);
	if (!(*field)) {
		fprintf(stderr, "? no malloc space\n");
		exit(10);
	}
	memset(*field, 0, len);
	strncpy(*field, event->data.scalar.value, event->data.scalar.length);
}

struct dsd_device *parse_dev_doc(char *buf)
{
	yaml_parser_t parser;
	yaml_event_t event;
	int state;
	int doc_ok;
	struct dsd_device *device = NULL;
	struct dsd_device *retval = NULL;
	struct dsd_device_property *dprop = NULL;

	/* initialize the parser */
	if (!yaml_parser_initialize(&parser)) {
		fprintf(stderr, "? cannot initialize parser\n");
		exit(2);
	}

	/* tell the parser what to read */
	yaml_parser_set_input_string(&parser, buf, strlen(buf));

	/* do something with the content */
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
			break;
		case YAML_STREAM_END_EVENT:
			break;

		/* block delimiters */
		case YAML_DOCUMENT_START_EVENT:
			state = 0;		/* start state machine */
			doc_ok = 1;
			printf("---\n");
			device = malloc(sizeof(struct dsd_device));
			if (!device) {
				fprintf(stderr, "? cannot malloc device\n");
				exit(1);
			}
			memset(device, 0, sizeof(struct dsd_device));
			break;
		case YAML_DOCUMENT_END_EVENT:
			if (doc_ok)
				retval = device;
			else {
				free(device);
				retval = NULL;
			}
			event.type = YAML_STREAM_END_EVENT;
			break;

		case YAML_SEQUENCE_START_EVENT:
			//printf("\t\t-> Sequence Start\n");
			if (state == 100) {
				state = 101;
				TAILQ_INIT(&device->properties);
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
			if (state == 0)
				state = 0;
			else if (state == 101) {
				state = 101;
				dprop = malloc(
					   sizeof(struct dsd_device_property));
				if (!dprop) {
					fprintf(stderr,
					   "? cannot malloc device property\n");
					exit(1);
				}
				memset(dprop, 0,
				       sizeof(struct dsd_property_value));
			} else {
				fprintf(stderr, "? expected a token: entry\n");
				event.type = YAML_STREAM_END_EVENT;
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
					 TOK_DEVICE)) {
					printf("\tdevice: ");
					state = 1;
				}
				else if (!strcasecmp(event.data.scalar.value,
					 TOK_DESCRIPTION)) {
					printf("\tdescription: ");
					state = 2;
				}
				else if (!strcasecmp(event.data.scalar.value,
					 TOK_OWNER)) {
					printf("\towner: ");
					state = 3;
				}
				else if (!strcasecmp(event.data.scalar.value,
					 TOK_PROPERTIES)) {
					printf("\tproperties:\n");
					state = 100;
				}
				else {
					fprintf(stderr,
						"? unknown keyword: %s\n",
						event.data.scalar.value);
					exit(1);
				}
				break;
			case 1:
				printf("%s\n", event.data.scalar.value);
				set_value(&device->device, &event);
				state = 0;
				break;
			case 2:
				printf("\t%s\n", event.data.scalar.value);
				set_value(&device->description, &event);
				state = 0;
				break;
			case 3:
				printf("%s\n", event.data.scalar.value);
				set_value(&device->owner, &event);
				state = 0;
				break;
			/* state 100: see sequence start */
			case 101:
				printf("\t\t- %s\n", event.data.scalar.value);
				dprop = malloc(
					sizeof(struct dsd_device_property));
				if (!dprop) {
					fprintf(stderr,
					   "? cannot malloc device property\n");
					exit(1);
				}
				memset(dprop, 0,
				       sizeof(struct dsd_device_property));
				set_value(&dprop->property, &event);
				TAILQ_INSERT_TAIL(&(device->properties),
						  dprop, entries);
				state = 101;
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

	return retval;
}

struct dsd_property *parse_prop_doc(char *buf)
{
	yaml_parser_t parser;
	yaml_event_t event;
	int state;
	int doc_ok;
	struct dsd_property *property = NULL;
	struct dsd_property *retval = NULL;
	struct dsd_property_value *value = NULL;
	struct dsd_device_name *dname = NULL;

	/* initialize the parser */
	if (!yaml_parser_initialize(&parser)) {
		fprintf(stderr, "? cannot initialize parser\n");
		exit(2);
	}

	/* tell the parser what to read */
	yaml_parser_set_input_string(&parser, buf, strlen(buf));

	/* do something with the content */
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
			break;
		case YAML_STREAM_END_EVENT:
			break;

		/* block delimiters */
		case YAML_DOCUMENT_START_EVENT:
			/* start a property document */
			printf("---\n");
			property = malloc(sizeof(struct dsd_property));
			if (!property) {
				fprintf(stderr, "? cannot malloc property\n");
				exit(1);
			}
			memset(property, 0, sizeof(struct dsd_property));
			state = 0;		/* start state machine */
			break;
		case YAML_DOCUMENT_END_EVENT:
			if (doc_ok)
				retval = property;
			else {
				free(property);
				retval = NULL;
				event.type = YAML_STREAM_END_EVENT;
			}
			state = 0;
			break;

		case YAML_SEQUENCE_START_EVENT:
			//printf("\t\t-> Sequence Start\n");
			if (state == 100) {
				state = 101;
				TAILQ_INIT(&property->values);
			} else if (state == 200) {
				state = 201;
				TAILQ_INIT(&property->devices);
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
			if (state == 0)
				state = 0;
			else if (state == 101) {
				state = 102;
				value = malloc(sizeof(struct dsd_property_value));
				if (!value) {
					fprintf(stderr, "? cannot malloc property value\n");
					exit(1);
				}
				memset(value, 0, sizeof(struct dsd_property_value));
			} else if (state == 201) {
				state = 201;
				dname = malloc(sizeof(struct dsd_device_name));
				if (!dname) {
					fprintf(stderr,
					    "? cannot malloc device name\n");
					exit(1);
				}
				memset(dname, 0,
				       sizeof(struct dsd_device_name));
			} else {
				fprintf(stderr, "? expected a token: entry\n");
				event.type = YAML_STREAM_END_EVENT;
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
					state = 100;
				}
				else if (!strcasecmp(event.data.scalar.value,
					 TOK_DEVICES)) {
					printf("\tdevices:\n");
					state = 200;
				}
				else {
					fprintf(stderr,
						"? unknown keyword: %s\n",
						event.data.scalar.value);
					exit(1);
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
			/* case 100-101: see sequence start ... */
			case 102:
				if (!strcasecmp(event.data.scalar.value,
				    TOK_TOKEN)) {
					printf("\t\t- token: ");
					state = 103;
				} else {
					state = 0;
				}
				break;
			case 103:
				printf("%s\n", event.data.scalar.value);
				set_value(&value->token, &event);
				state = 104;
				break;
			case 104:
				if (!strcasecmp(event.data.scalar.value,
				    TOK_DESCRIPTION)) {
					printf("\t\t  description: ");
					state = 105;
				}
				break;
			case 105:
				printf("%s\n", event.data.scalar.value);
				set_value(&value->description, &event);
				TAILQ_INSERT_TAIL(&(property->values),
						  value, entries);
				state = 101;
				break;
			/* case 200: see sequence start ... */
			case 201:
				printf("\t\t- %s\n", event.data.scalar.value);
				dname = malloc(sizeof(struct dsd_device_name));
				if (!dname) {
					fprintf(stderr,
					    "? cannot malloc device name\n");
					exit(1);
				}
				memset(dname, 0,
				       sizeof(struct dsd_device_name));
				set_value(&dname->name, &event);
				if (db_dev_lookup(dname->name)) {
					if (find_dev(dname->name)) {
						fprintf(stderr,
						   "? no such device: %s\n",
						   dname->name);
						exit(1);
					}
				}
				TAILQ_INSERT_TAIL(&(property->devices),
						  dname, entries);
				state = 201;
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

	return retval;
}

