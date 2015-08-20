#ifndef __LOCAL_PARSE_H
#define __LOCAL_PARSE_H
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

#define TOK_DESCRIPTION	"description"
#define TOK_EXAMPLE	"example"

#define TOK_DEVICE	"device"
#define TOK_DEVICES	"devices"
#define TOK_OWNER	"owner"
#define TOK_PROPERTY	"property"
#define TOK_PROPERTIES	"properties"
#define TOK_TOKEN	"token"
#define TOK_TYPE	"type"
#define TOK_VALUES	"values"

#define TYPE_HEX_ADDR_PKG	"hexadecimal-address-package"
#define TYPE_HEX_INTEGER	"hexadecimal-integer"
#define TYPE_INTEGER		"integer"
#define TYPE_STRING		"string"

enum dsd_doc_type {
	DUNNO,
	DEVDOC,
	PROPDOC
};

extern struct dsd_device *parse_dev_doc(char *buf);
extern struct dsd_property *parse_prop_doc(char *buf);

#endif
