#!/usr/bin/env python3
#
# Copyright (c) 2106, Al Stone <ahs3@redhat.com>
#

import os.path
import shlex

# character token values
COLON           = ":"
COMMA           = ","
EOL             = "\n"
LBRACE          = "{"
RBRACE          = "}"
SLASH           = "/"

# tokens for reserved keywords
ACKED_BY        = "acked-by"
BUS             = "bus"
DESCRIPTION     = "description"
DERIVED_FROM    = "derived-from"
DEVICE_ID       = "device-id"
EXAMPLE         = "example"
PROPERTY        = "property"
PROPERTY_SET    = "property-set"
REQUIRES        = "requires"
REVIEWED_BY     = "reviewed-by"
REVISION        = "revision"
SET_TYPE        = "set-type"
SUBMITTED_BY    = "submitted-by"
SUBPACKAGE      = "subpackage"
TOKEN           = "token"
TYPE            = "type"
USAGE           = "usage"
VALUE           = "value"
VENDOR          = "vendor"

# tokens for bus
SHARED          = "shared"

# tokens for set-type values
ABSTRACT        = "abstract"
DEFINITION      = "definition"
SUBSET          = "subset"

# tokens for type values (INTEGER, REFERENCE can be keywords, in context)
INTEGER         = "integer"
PACKAGE         = "package"
REFERENCE       = "reference"
STRING          = "string"

# tokens for usage values
OPTIONAL        = "optional"
REQUIRED        = "required"

# misc tokens
NAME            = "name"

# reserved word token list
KEYWORDS = [ ACKED_BY, BUS, DESCRIPTION, DERIVED_FROM, DEVICE_ID, 
             EXAMPLE, PROPERTY, PROPERTY_SET, REQUIRES, REVIEWED_BY,
             REVISION, SET_TYPE, SUBMITTED_BY, SUBPACKAGE, TOKEN, 
             TYPE, USAGE, VALUE, VENDOR,
           ]

class Parser:
    def __init__(self, fname):
        self.error = True
        self.fname = fname
        self.text = ""
        self.lines = 0
        self.errors = 0
        self.properties = {}
        self.attrs = {}

        if not os.path.exists(fname):
            self.error = True
            print("? cannot find file: %s" % (fname))
            return

        with open(fname, "r") as f:
            self.text = f.readlines()
            f.close()

        self.lines = len(self.text)
        self.parse()
        return

    def dump(self):
        print("+------------------------------------------------------+")
        print("| File: %s" % (self.fname))
        print("+------------------------------------------------------+")
        
        for ii in self.text:
            print("%s" % (ii), end='')

        print("+------------------------------------------------------+")
        print(self.attrs)
        print(self.properties)

        return

    def has_errors(self):
        return self.error

    def parse(self):
        linenum = 0

        lexer = shlex.shlex(''.join(self.text), posix=True)
        lexer.infile = self.fname
        lexer.wordchars = lexer.wordchars + '-./'
        lexer.whitespace = " \t"
        self.error = False
        tok = lexer.get_token()
        state = 0
        while tok and not self.error:
            s = tok
            if s == "\n":
                s = "EOL"
            print("state, token => %d, %s" % (state, s))
            if state == 0:
                if tok == PROPERTY_SET:
                    attr = PROPERTY_SET
                    state = 1
                else:
                    self.error = True
                    self.errors += 1
                    print("%s%s is required as first line" %
                          (lexer.error_leader(), PROPERTY_SET))

            elif state == 1:
                if tok == COLON:
                    state = 2
                else:
                    self.error = True
                    self.errors += 1
                    print("%scolon required after %s" %
                          (lexer.error_leader(), attr))

            elif state == 2:
                if len(tok) > 0:
                    if attr not in self.attrs.keys():
                        self.attrs[attr] = {}
                    self.attrs[attr] = tok
                    state = 3

            elif state == 3:
                if tok != EOL:
                    if attr not in self.attrs.keys():
                        self.attrs[attr] = {}
                    self.attrs[attr] += ' ' + tok
                    state = 3
                else:
                    state = 4

            elif state == 4:
                if tok == EOL:
                    state = 4
                elif tok == DESCRIPTION:
                    attr = tok
                    state = 5
                elif tok == PROPERTY:
                    attr = tok
                    state = 7
                elif tok in KEYWORDS:
                    attr = tok
                    state = 1
                else:
                    self.error = True
                    self.errors += 1
                    print("%skeyword expected instead of %s" %
                          (lexer.error_leader(), tok))

            elif state == 5:
                if tok == COLON:
                    state = 5
                if tok == EOL:
                    state = 6
                else:
                    if prop not in self.properties.keys():
                        self.properties[prop] = {}
                    self.properties[prop][DESCRIPTION] = tok

            elif state == 6:
                if tok == EOL:
                    state = 4
                else:
                    if prop not in self.properties.keys():
                        self.properties[prop] = {}
                    self.properties[prop][DESCRIPTION] += " " + tok

            elif state == 7:
                if tok == COLON:
                    state = 8
                else:
                    self.error = True
                    self.errors += 1
                    print("%scolon required after %s" %
                          (lexer.error_leader(), attr))

            elif state == 8:
                prop = tok
                if prop not in self.properties.keys():
                    self.properties[prop] = {}
                self.properties[prop][NAME] = prop 
                state = 4

            else:
                print("? internal error: state %d, token %s" % (state, tok))
                self.error = True
                self.errors += 1
                break

            if not self.error:
                tok = lexer.get_token()

        print("%s lines read" % (self.lines))
        print("%s lines parsed, %d errors found" %
              (lexer.lineno - 1, self.errors))
        return self.error
