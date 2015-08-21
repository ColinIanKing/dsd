#ifndef __LOCAL_DB_H
#define __LOCAL_DB_H
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

#include "dsd.h"

int db_cat(char *propname);			/* write property to stdout */
int db_close(char *dirname);			/* close the open "data base" */
int db_delete(char *propname);			/* remove property from db */
int db_dev_list(void);				/* list all devices */
int db_dev_lookup(char *name);			/* look for device */
int db_init(char *dirname);			/* create a "data base" */
int db_list(void);				/* list all entries */
int db_lookup(char *name);			/* look for property or dev */
int db_open(char *dirname);			/* open the "data base" */
int db_prop_list(void);				/* list all properties */
int db_prop_lookup(char *name);			/* look for property */
int db_dev_write(struct dsd_device *dev);	/* write a device into db */
int db_prop_write(struct dsd_property *prop);	/* write a property into db */
int db_verify(char *dirname);			/* check consistency of db */

#endif
