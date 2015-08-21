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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/param.h>
#include <sys/stat.h>

#include "db.h"
#include "dsd.h"
#include "parse.h"
#include "output.h"


int simple_text_device(struct dsd_device *dev)
{
	const int WIDTH = 17;
	const char blanks[] = "                ";

	/* write simple text for the device to stdout */
	fprintf(stdout, "%-*s %s\n", WIDTH, "Device Name:", dev->device);

	if (dev->owner)
		fprintf(stdout, "%-*s %s\n", WIDTH, "Maintainer:", dev->owner);

	if (dev->description)
		fprintf(stdout, "%-*s %s\n", WIDTH,
			"Description:", dev->description);

	if (!TAILQ_EMPTY(&dev->properties)) {
		struct dsd_device_property *dvp;
		int n;

		fprintf(stdout, "%-*s", WIDTH, "Uses Properties:");
		n = 1;
		TAILQ_FOREACH(dvp, &dev->properties, entries) {
			if (n == 1)
				fprintf(stdout, " %s\n", dvp->property);
			else
				fprintf(stdout, "%-*s %s\n",
					WIDTH, blanks,
					dvp->property);
			n++;
		}
	}

	return 0;
}

int simple_text_property(struct dsd_property *prop)
{
	const int WIDTH = 17;
	const char blanks[] = "                ";

	char *tmp;

	/* write simple text for the property to stdout */
	fprintf(stdout, "%-*s %s\n", WIDTH, "Property:", prop->property);

	if (prop->owner)
		fprintf(stdout, "%-*s %s\n", WIDTH, "Maintainer:",
			prop->owner);

	if (prop->description) {
		fprintf(stdout, "%-*s\n", WIDTH, "Description:");
		tmp = strtok(prop->description, "\n");
		while (tmp) {
			fprintf(stdout, "    %s\n", tmp);
			tmp = strtok(NULL, "\n");
		}
	}
	fprintf(stdout, "\n");

	if (prop->type)
		fprintf(stdout, "%-*s %s\n", WIDTH, "Type:", prop->type);

	if (!TAILQ_EMPTY(&prop->values)) {
		struct dsd_property_value *pvp;
		int n, spaces;

		spaces = 0;
		TAILQ_FOREACH(pvp, &prop->values, entries) {
			if (strlen(pvp->token) > spaces)
				spaces = strlen(pvp->token);
		}

		fprintf(stdout, "%-*s", WIDTH, "Possible Values:");
		n = 1;
		TAILQ_FOREACH(pvp, &prop->values, entries) {
			if (n == 1)
				fprintf(stdout, " %-*s => %s\n",
					spaces, pvp->token,
					pvp->description);
			else
				fprintf(stdout, "%-*s %-*s => %s\n",
					WIDTH, blanks,
					spaces, pvp->token,
					pvp->description);
			n++;
		}
	}

	if (!TAILQ_EMPTY(&prop->devices)) {
		struct dsd_device_name *dnp;
		int n;

		fprintf(stdout, "%-*s ", WIDTH, "Used by Devices:");
		n = 1;
		TAILQ_FOREACH(dnp, &prop->devices, entries) {
			if (n == 1)
				fprintf(stdout, "%s\n", dnp->name);
			else
				fprintf(stdout, "%-*s %s\n",
					WIDTH, blanks, dnp->name);
			n++;
		}
	}

	if (prop->example) {
		fprintf(stdout, "%-*s\n", WIDTH, "Example:");
		tmp = strtok(prop->example, "\n");
		while (tmp) {
			fprintf(stdout, "    %s\n", tmp);
			tmp = strtok(NULL, "\n");
		}
	}
	fprintf(stdout, "\n");

	return 0;
}

int simple_text(char *name)
{
	struct dsd_device *dev;
	struct dsd_property *prop;

	if (!name)
		return 1;

	if (!db_dev_lookup(name)) {
		dev = db_get_device(name);
		if (!dev)
			return 1;
		simple_text_device(dev);
		return 0;
	}

	if (!db_prop_lookup(name)) {
		prop = db_get_property(name);
		if (!prop)
			return 1;
		simple_text_property(prop);
		return 0;
	}

	return 1;	/* not found */
}
