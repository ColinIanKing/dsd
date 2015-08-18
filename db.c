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

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/param.h>
#include <sys/stat.h>

#include "dsd.h"
#include "db.h"

static char *dbname = NULL;

int db_init(char *dirname)
{
	/* create a "data base": for now, this just means a directory */
	return mkdir(dirname, S_IRWXU | S_IRWXG);
}

int db_open(char *dirname)
{
	struct stat sb;

	/* just make sure it exists for now */
	stat(dirname, &sb);
	if (!S_ISDIR(sb.st_mode))
		return 1;

	if (dbname) {
		free(dbname);
		dbname = NULL;
	}
	dbname = malloc(strlen(dirname) + 1);
	if (!dbname)
		return 1;

	strcpy(dbname, dirname);
	return 0;
}

int db_close(char *dirname)
{
	if (dbname)
		free(dbname);
	dbname = NULL;
	return 0;
}

static char *build_path(char *propname)
{
	char *path;

	path = malloc(3 * MAXPATHLEN);
	if (!path) {
		fprintf(stderr, "? path malloc failed\n");
		exit(1);
	}

	memset(path, 0, 3 * MAXPATHLEN);
	strcpy(path, dbname);
	strcat(path, "/");
	strcat(path, propname);

	return path;
}

static void free_path(char *path)
{
	if (path)
		free(path);
}

int db_lookup(char *propnam)
{
	struct stat sb;
	char *path;

	/* see if the property named is in the db */
	path = build_path(propnam);
	if (!dbname) {
		fprintf(stderr, "? must invoke db_open first\n");
		exit(1);
	}

	stat(path, &sb);
	free_path(path);
	if (!S_ISREG(sb.st_mode))
		return 1;

	return 0;
}

int db_write(struct dsd_property *prop)
{
	FILE *fp;
	char *path;
	char *tmp;

	/* write the property to the db */
	path = build_path(prop->property);
	fp = fopen(path, "w");
	if (!fp) {
		fprintf(stderr, "? cannot open property: %s\n", prop->property);
		exit(1);
	}

	fprintf(fp, "property: %s\n", prop->property);

	if (prop->type)
		fprintf(fp, "type: %s\n", prop->type);

	if (prop->owner)
		fprintf(fp, "owner: %s\n", prop->owner);

	if (prop->description) {
		fprintf(fp, "description: |\n");
		tmp = strtok(prop->description, "\n");
		while (tmp) {
			fprintf(fp, "\t%s\n", tmp);
			tmp = strtok(NULL, "\n");
		}
	}

	if (prop->example) {
		fprintf(fp, "example: |\n");
		tmp = strtok(prop->example, "\n");
		while (tmp) {
			fprintf(fp, "\t%s\n", tmp);
			tmp = strtok(NULL, "\n");
		}
	}

	if (!TAILQ_EMPTY(&prop->values)) {
		struct dsd_property_value *pvp;

		fprintf(fp, "values:\n");
		TAILQ_FOREACH(pvp, &prop->values, entries) {
			fprintf(fp, "\t- token: %s\n", pvp->token);
			fprintf(fp, "\t  description: %s\n", pvp->description);
		}
	}

	fclose(fp);
	free_path(path);
	return 0;
}

int db_cat(char *propname)
{
	FILE *fp;
	char *path;
	char *tmp;
	size_t len;

	/* write the property info to stdout */
	path = build_path(propname);
	fp = fopen(path, "r");
	if (!fp) {
		fprintf(stderr, "? cannot open property: %s\n", propname);
		exit(1);
	}

	tmp = malloc(MAXPATHLEN);
	memset(tmp, 0, MAXPATHLEN);
	fprintf(stdout, "%s\n", "---");	/* document start */
	while (!feof(fp)) {
		len = fread(tmp, 1, MAXPATHLEN, fp);
		if (len > 0)
			fwrite(tmp, 1, len, stdout);
		memset(tmp, 0, MAXPATHLEN);
	}

	fclose(fp);
	free_path(path);
	return 0;
}

int db_delete(char *propname)
{
	char *path;

	/* delete the property from the db */
	path = build_path(propname);
	if (unlink(path)) {
		fprintf(stderr, "? cannot delete property: %s\n", propname);
		exit(1);
	}
	free_path(path);
	return 0;
}

int db_list(void)
{
	DIR *dirp;
	struct dirent *dep;

	/*
	 * list all the files in the db -- this is logically
	 * equivalent to listing all the property names
	 */
	if (!dbname) {
		fprintf(stderr, "? must open db first\n");
		exit(1);
	}
	dirp = opendir(dbname);
	if (!dirp) {
		fprintf(stderr, "? cannot open db directory\n");
		exit(1);
	}

	dep = readdir(dirp);
	while (dep) {
		if (strcmp(dep->d_name, ".") && strcmp(dep->d_name, ".."))
			printf("%s\n", dep->d_name);
		dep = readdir(dirp);
	}

	closedir(dirp);
	return 0;
}
