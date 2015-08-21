#ifndef __LOCAL_DSD_H
#define __LOCAL_DSD_H
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

#include <sys/queue.h>

#define CMD_ADD		"add"
#define CMD_DELETE	"delete"
#define CMD_HELP	"help"
#define CMD_INIT	"init"
#define CMD_LIST	"list"
#define CMD_LOOKUP	"lookup"
#define CMD_VERIFY	"verify"

#define SUBCMD_ALL	"all"
#define SUBCMD_DEVS	"devs"
#define SUBCMD_PROPS	"props"

enum dsd_command {
	dsd_add = 0,
	dsd_delete,
	dsd_help,
	dsd_init,
	dsd_list,
	dsd_lookup,
	dsd_verify,
	dsd_undefined
};

TAILQ_HEAD(dsd_property_queue_head, dsd_property) dpqhead;
extern struct dsd_property_queue_head *dpqheadp;

struct dsd_property_value {
	char	*token;
	char	*description;
	TAILQ_ENTRY(dsd_property_value) entries;
};

struct dsd_device_name {
	char	*name;
	TAILQ_ENTRY(dsd_device_name) entries;
};

struct dsd_property {
	char	*property;
	char	*type;
	char	*owner;
	TAILQ_HEAD(dsd_device_name_head, dsd_device_name) devices;
	char	*description;
	char	*example;
	TAILQ_HEAD(dsd_property_value_head, dsd_property_value) values;
	TAILQ_ENTRY(dsd_property) entries;
};

TAILQ_HEAD(dsd_device_queue_head, dsd_device) ddqhead;
extern struct dsd_device_queue_head *ddqheadp;

struct dsd_device_property {
	char	*property;
	TAILQ_ENTRY(dsd_device_property) entries;
};

struct dsd_device {
	char	*device;
	char	*owner;
	char	*description;
	TAILQ_HEAD(dsd_device_property_head, dsd_device_property) properties;
	TAILQ_ENTRY(dsd_device) entries;
};

#endif
