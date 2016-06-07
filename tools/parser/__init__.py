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
VALUES          = "values"
VENDOR          = "vendor"

# tokens for bus
SHARED          = "shared"

# tokens for set-type values
ABSTRACT        = "abstract"
DEFINITION      = "definition"
SUBSET          = "subset"
SET_TYPES       = [ ABSTRACT, DEFINITION, SUBSET ]

# tokens for type values (INTEGER, REFERENCE can be keywords, in context)
INTEGER         = "integer"
PACKAGE         = "package"
REFERENCE       = "reference"
STRING          = "string"
VALUE_TYPES     = [ INTEGER, PACKAGE, REFERENCE, STRING ]

# tokens for usage values
OPTIONAL        = "optional"
REQUIRED        = "required"

# misc tokens
NAME            = "name"
RANGE           = "range"

# reserved word token list
KEYWORDS = [ ACKED_BY, BUS, DESCRIPTION, DERIVED_FROM, DEVICE_ID, 
             EXAMPLE, PROPERTY, PROPERTY_SET, REQUIRES, REVIEWED_BY,
             REVISION, SET_TYPE, SUBMITTED_BY, SUBPACKAGE, TOKEN, 
             TYPE, USAGE, VALUES, VENDOR,
           ]
SET_KEYWORDS = [ ACKED_BY, BUS, DESCRIPTION, DERIVED_FROM, DEVICE_ID, 
                 EXAMPLE, PROPERTY, PROPERTY_SET, REQUIRES, REVIEWED_BY,
                 REVISION, SET_TYPE, SUBMITTED_BY, VENDOR,
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

        print("\n")
        print("+------------------------------------------------------+")
        print("| Attributes                                           +")
        print("+------------------------------------------------------+")
        print(self.attrs)

        print("\n")
        print("+------------------------------------------------------+")
        print("| Properties                                           +")
        print("+------------------------------------------------------+")
        for ii in self.properties.keys():
            print("==== %s ====" % (ii))
            print(self.properties)

        return

    def has_errors(self):
        return self.error

    def __look_for_colon(self, attr, tok, lexer, errcnt, ok_state):
        errflag = False
        if tok == COLON:
            state = ok_state
        else:
            errflag = True
            errcnt += 1
            state = 0
            print("%scolon required after %s" % (lexer.error_leader(), attr))
        return (errflag, errcnt, state)

    def __get_raw_oneline(self, lexer, tok):
        text = ""
        while tok != EOL:
            text += tok + " "
            tok = lexer.get_token()
            if tok in KEYWORDS:
                lexer.push_token(tok)
                break
        print("=> one line text/%d: %s" % (len(text.split("\n")), text.strip()))
        return text.strip()

    def __get_raw_multiline(self, lexer, tok):
        text = ""
        if tok != EOL:
            text = tok + " "
        state = 0
        c = lexer.instream.read(1)
        while True:
            if state == 0:
                if c == "\n":
                    state = 1
                else:
                    text += c
                    c = lexer.instream.read(1)
            
            elif state == 1:
                tok = lexer.get_token()
                if tok in KEYWORDS:
                    lexer.push_token(tok)
                    break
                else:
                    text += " " + tok
                    state = 0

        lexer.lineno += 1
        print("=> text/%d: %s" % (len(text.split("\n")), text.strip()))
        return text.strip()

    def parse(self):
        linenum = 0

        lexer = shlex.shlex(''.join(self.text), posix=True)
        lexer.infile = self.fname
        lexer.wordchars = lexer.wordchars + '-./'
        lexer.whitespace = " \t"
        lexer.quotes = ""
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
                self.error, self.errors, state = \
                    self.__look_for_colon(attr, tok, lexer, self.errors, 2)

            elif state == 2:
                text = self.__get_raw_oneline(lexer, tok)
                if attr not in self.attrs.keys():
                    self.attrs[attr] = {}
                self.attrs[attr] = text
                state = 3

            elif state == 3:
                if tok == EOL:
                    state = 3
                elif tok in SET_KEYWORDS:
                    attr = tok
                    state = 4

            elif state == 4:
                self.error, self.errors, state = \
                    self.__look_for_colon(attr, tok, lexer, self.errors, 5)

            elif state == 5:
                if attr not in self.attrs.keys():
                    self.attrs[attr] = {}

                if attr == SET_TYPE:
                    if tok in SET_TYPES:
                        self.attrs[attr] = tok
                        state = 3
                    else:
                        self.error = True
                        self.errors += 1
                        print("%sundefined property-set type: %s" %
                            (lexer.error_leader(), tok))

                elif attr == DERIVED_FROM or attr == ACKED_BY or \
                     attr == SUBMITTED_BY or attr == REVIEWED_BY:
                    if len(self.attrs[attr]) < 1:
                        self.attrs[attr] = []
                    text = self.__get_raw_oneline(lexer, tok)
                    self.attrs[attr].append(text.strip())
                    state = 3

                elif attr == VENDOR or attr == BUS or attr == DEVICE_ID or \
                     attr == REVISION:
                    text = self.__get_raw_oneline(lexer, tok)
                    self.attrs[attr] = text
                    state = 3

#           elif state == 7:
#               self.error, self.errors, state = \
#                   self.__look_for_colon(attr, tok, lexer, self.errors, 8)

#           elif state == 8:
#               prop = tok
#               if prop not in self.properties.keys():
#                   self.properties[prop] = {}
#               self.properties[prop][NAME] = prop 
#               state = 4

#           elif state == 9:
#               self.error, self.errors, state = \
#                   self.__look_for_colon(attr, tok, lexer, self.errors, 10)

#           elif state == 10:
#               if prop not in self.properties.keys():
#                   self.properties[prop] = {}
#               self.properties[prop][TYPE] = tok 
#               prop_type = tok
#               state = 4

#           elif state == 11:
#               self.error, self.errors, state = \
#                   self.__look_for_colon(attr, tok, lexer, self.errors, 12)
#               
#           elif state == 12:
#               if tok == EOL:
#                   state = 13
#               elif tok != EOL:
#                   self.error = True
#                   self.errors += 1
#                   print("%sEOL expected after %s" %
#                         (lexer.error_leader(), attr))

#           elif state == 13:
#               if tok in KEYWORDS:
#                   state = 4
#                   continue
#               value = {}
#               value[TYPE] = tok
#               if prop_type == STRING:
#                   if tok == TOKEN:
#                       state = 14
#                   else:
#                       self.error = True
#                       self.errors += 1
#                       print("%sexpected the 'token' keyword" %
#                             (lexer.error_leader()))
#               elif prop_type == INTEGER:
#                   if tok == INTEGER:
#                       state = 15
#                   else:
#                       self.error = True
#                       self.errors += 1
#                       print("%sexpected the 'integer' keyword" %
#                             (lexer.error_leader()))
#               elif prop_type == REFERENCE:
#                   if tok == REFERENCE:
#                       state = 16
#                   else:
#                       self.error = True
#                       self.errors += 1
#                       print("%sexpected the 'reference' keyword" %
#                             (lexer.error_leader()))
#               elif prop_type == PACKAGE:
#                   if tok == SUBPACKAGE:
#                       state = 17
#                   else:
#                       self.error = True
#                       self.errors += 1
#                       print("%sexpected the 'subpackage' keyword" %
#                             (lexer.error_leader()))
#               else:
#                   self.error = True
#                   self.errors += 1
#                   print("%sunknown value type: %s" %
#                         (lexer.error_leader(), prop_type))

#           elif state == 14:
#               self.error, self.errors, state = \
#                   self.__look_for_colon(attr, tok, lexer, self.errors, 15)

#           elif state == 15:
#               if tok == EOL:
#                   self.error = True
#                   self.errors += 1
#                   print("%sexpected but did not find a value" %
#                         (lexer.error_leader()))
#               else:
#                   value[RANGE] = tok
#                   state = 16

#           elif state == 16:
#               if tok != EOL:
#                   value[RANGE] += " " + tok
#                   state = 16
#               else:
#                   state = 17

#           elif state == 17:
#               if tok == DESCRIPTION:
#                   state = 18
#               else:
#                   self.error = True
#                   self.errors += 1
#                   print("%sexpected but did not find 'description'" %
#                         (lexer.error_leader()))
#               
#           elif state == 18:
#               self.error, self.errors, state = \
#                   self.__look_for_colon(attr, tok, lexer, self.errors, 19)

#           elif state == 19:
#               if tok == EOL:
#                   self.error = True
#                   self.errors += 1
#                   print("%sexpected but did not find a description" %
#                         (lexer.error_leader()))
#               else:
#                   text = self.__get_raw_text(lexer, tok)
#                   value[DESCRIPTION] = text
#                   self.properties[prop][VALUES].append(value)
#                   state = 13

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
