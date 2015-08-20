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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/param.h>
#include <sys/stat.h>

#include "dsd.h"
#include "db.h"

#define PROPS		"properties"
#define DEVS		"devices"

static char *dbname = NULL;

int db_init(char *dirname)
{
	int err;
	char *path;

	/*
	 * create a "data base": for now, this means create a directory
	 * and within it two subdirectories
	 */
	err = mkdir(dirname, S_IRWXU | S_IRWXG);
	if (err) {
		perror("? init mkdir failed");
		return errno;
	}

	path = malloc(3 * MAXPATHLEN);
	if (!path) {
		fprintf(stderr, "? init path malloc failed\n");
		return errno;
	}

	memset(path, 0, 3 * MAXPATHLEN);
	strcpy(path, dirname);
	strcat(path, "/");
	strcat(path, PROPS);
	err = mkdir(path, S_IRWXU | S_IRWXG);
	if (err) {
		perror("? init mkdir props failed");
		return errno;
	}

	memset(path, 0, 3 * MAXPATHLEN);
	strcpy(path, dirname);
	strcat(path, "/");
	strcat(path, DEVS);
	err = mkdir(path, S_IRWXU | S_IRWXG);
	if (err) {
		perror("? init mkdir devs failed");
		return errno;
	}

	return 0;
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

static char *__build_path(char *prefix, char *name)
{
	char *path;

	path = malloc(4 * MAXPATHLEN);
	if (!path) {
		fprintf(stderr, "? path malloc failed\n");
		exit(1);
	}

	memset(path, 0, 4 * MAXPATHLEN);
	strcpy(path, dbname);
	strcat(path, "/");
	strcat(path, prefix);
	if (name) {
		strcat(path, "/");
		strcat(path, name);
	}

	return path;
}

static char *build_dev_path(char *devname)
{
	return __build_path(DEVS, devname);
}

static char *build_prop_path(char *propname)
{
	return __build_path(PROPS, propname);
}

static void free_path(char *path)
{
	if (path)
		free(path);
}

int db_dev_lookup(char *name)
{
	struct stat sb;
	char *path;

	/* see if the device named is in the db */
	path = build_dev_path(name);
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

int db_prop_lookup(char *name)
{
	struct stat sb;
	char *path;

	/* see if the property named is in the db */
	path = build_prop_path(name);
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

int db_lookup(char *name)
{
	if (!db_prop_lookup(name))
		return 0;

	if (!db_dev_lookup(name))
		return 0;

	return 1;
}

int db_dev_write(struct dsd_device *dev)
{
	FILE *fp;
	char *path;
	char *tmp;

	/* write the device to the db */
	path = build_dev_path(dev->device);
	fp = fopen(path, "w");
	if (!fp) {
		fprintf(stderr, "? cannot open device: %s\n", dev->device);
		exit(1);
	}

	fprintf(fp, "device: %s\n", dev->device);

	if (dev->owner)
		fprintf(fp, "owner: %s\n", dev->owner);

	if (dev->description) {
		fprintf(fp, "description: |\n");
		tmp = strtok(dev->description, "\n");
		while (tmp) {
			fprintf(fp, "\t%s\n", tmp);
			tmp = strtok(NULL, "\n");
		}
	}

	if (!TAILQ_EMPTY(&dev->properties)) {
		struct dsd_device_property *dvp;

		fprintf(fp, "properties:\n");
		TAILQ_FOREACH(dvp, &dev->properties, entries) {
			fprintf(fp, "\t- %s\n", dvp->property);
		}
	}

	fclose(fp);
	free_path(path);
	return 0;
}

int db_prop_write(struct dsd_property *prop)
{
	FILE *fp;
	char *path;
	char *tmp;

	/* write the property to the db */
	path = build_prop_path(prop->property);
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

	if (!TAILQ_EMPTY(&prop->devices)) {
		struct dsd_device_name *dnp;

		fprintf(fp, "devices:\n");
		TAILQ_FOREACH(dnp, &prop->devices, entries) {
			fprintf(fp, "\t- %s\n", dnp->name);
		}
	}

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

int db_cat(char *name)
{
	FILE *fp;
	char *path;
	char *tmp;
	size_t len;

	/* write info to stdout */
	path = build_prop_path(name);
	fp = fopen(path, "r");
	if (!fp) {		/* not a property, maybe a device? */
		free_path(path);
		path = build_dev_path(name);
		fp = fopen(path, "r");
		if (!fp) {
			fprintf(stderr, "? no such entry: %s\n", name);
			exit(1);
		}
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

int db_delete(char *name)
{
	char *ppath;
	char *dpath;

	/* delete the property from the db */
	ppath = build_prop_path(name);
	if (unlink(ppath)) {
		/* maybe it's a device, not a property */
		dpath = build_dev_path(name);
		if (unlink(dpath)) {
			fprintf(stderr, "? no such property or device: %s\n",
				name);
			exit(1);
		}
		free_path(dpath);
	}
	free_path(ppath);
	return 0;
}

int db_prop_list(void)
{
	DIR *dirp;
	struct dirent *dep;
	char *propdb;

	/*
	 * list all the properties in the db -- this is logically
	 * equivalent to listing all the property file names
	 */
	if (!dbname) {
		fprintf(stderr, "? must open db first\n");
		exit(1);
	}
	propdb = build_prop_path(NULL);
	dirp = opendir(propdb);
	if (!dirp) {
		fprintf(stderr, "? cannot open db directory\n");
		exit(1);
	}

	printf("-- all property names\n");
	dep = readdir(dirp);
	while (dep) {
		if (strcmp(dep->d_name, ".") && strcmp(dep->d_name, ".."))
			printf("%s\n", dep->d_name);
		dep = readdir(dirp);
	}

	closedir(dirp);
	return 0;
}

int db_dev_list(void)
{
	DIR *dirp;
	struct dirent *dep;
	char *devdb;

	/*
	 * list all the devices in the db -- this is logically
	 * equivalent to listing all the device file names
	 */
	if (!dbname) {
		fprintf(stderr, "? must open db first\n");
		exit(1);
	}
	devdb = build_dev_path(NULL);
	dirp = opendir(devdb);
	if (!dirp) {
		fprintf(stderr, "? cannot open db directory\n");
		exit(1);
	}

	printf("-- all device names\n");
	dep = readdir(dirp);
	while (dep) {
		if (strcmp(dep->d_name, ".") && strcmp(dep->d_name, ".."))
			printf("%s\n", dep->d_name);
		dep = readdir(dirp);
	}

	closedir(dirp);
	return 0;
}

int db_list(void)
{
	db_dev_list();
	db_prop_list();
	return 0;
}

