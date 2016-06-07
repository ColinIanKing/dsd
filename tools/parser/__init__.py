#!/usr/bin/env python3
#
# Copyright (c) 2106, Al Stone <ahs3@redhat.com>
#

import os
import os.path
import shlex

# character token values
COLON           = ":"
COMMA           = ","
EOL             = "\n"
GTTHAN          = ">"
LBRACE          = "{"
LESSTHAN        = "<"
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
PROPERTY_TYPES  = [ INTEGER, PACKAGE, REFERENCE, STRING ]

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
            print(self.properties[ii])

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
        state = 0
        while tok != EOL:
            if state == 0:
                text += " " + tok
                if tok == LESSTHAN:
                    state = 1

            elif state == 1:
                text += tok
                if tok == GTTHAN:
                    state = 0

            tok = lexer.get_token()
            if tok in KEYWORDS:
                lexer.push_token(tok)
                break
        #print("=> one line text/%d: %s" % (len(text.split("\n")), text.strip()))
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
        #print("=> text/%d: %s" % (len(text.split("\n")), text.strip()))
        return text.strip()

    def __get_comma_list(self, lexer, tok):
        plist = []
        while tok != EOL:
            if tok != COMMA:
                plist.append(tok)
            tok = lexer.get_token()
        return plist

    def parse(self):
        linenum = 0

        lexer = shlex.shlex(''.join(self.text), posix=True)
        lexer.infile = self.fname
        lexer.wordchars = lexer.wordchars + '-./'
        lexer.whitespace = " \t"
        lexer.quotes = ""
        self.error = False
        tok = lexer.get_token()
        attr = ""
        prop = ""
        state = 0
        while tok and not self.error:
            s = tok
            if s == "\n":
                s = "EOL"
            #print("state, token => %d, %s \t|| attr, prop => '%s', '%s'" % 
            #      (state, s, attr, prop))
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
                elif tok in KEYWORDS:
                    attr = tok
                    state = 4

            elif state == 4:
                self.error, self.errors, state = \
                    self.__look_for_colon(attr, tok, lexer, self.errors, 5)

            elif state == 5:
                if attr == SET_TYPE:
                    if attr not in self.attrs.keys():
                        self.attrs[attr] = {}
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
                    if attr not in self.attrs.keys():
                        self.attrs[attr] = []
                    text = self.__get_raw_oneline(lexer, tok)
                    self.attrs[attr].append(text.strip())
                    state = 3

                elif attr == VENDOR or attr == BUS or attr == DEVICE_ID or \
                     attr == REVISION:
                    if attr not in self.attrs.keys():
                        self.attrs[attr] = {}
                    text = self.__get_raw_oneline(lexer, tok)
                    self.attrs[attr] = text
                    state = 3

                elif attr == PROPERTY:
                    if prop and len(props) > 0:
                        self.properties[prop] = props.copy()
                        props.clear()
                    prop = tok
                    props = {}
                    state = 3

                elif attr == TYPE:
                    if not prop:
                        self.error = True
                        self.errors += 1
                        print("%sdo not have a property defined yet" %
                              (lexer.error_leader()))
                    else:
                        if tok in PROPERTY_TYPES:
                            props[TYPE] = tok
                            state = 3
                        else:
                            self.error = True
                            self.errors += 1
                            print("%sundefined property type: %s" %
                                  (lexer.error_leader(), tok))
                        
                elif attr == DESCRIPTION or attr == EXAMPLE:
                    if not prop:
                        self.error = True
                        self.errors += 1
                        print("%sdo not have a property defined yet" %
                              (lexer.error_leader()))
                    else:
                        text = self.__get_raw_multiline(lexer, tok)
                        props[attr] = text
                        state = 3

                elif attr == REQUIRES:
                    if not prop:
                        self.error = True
                        self.errors += 1
                        print("%sdo not have a property defined yet" %
                              (lexer.error_leader()))
                    else:
                        proplist = self.__get_comma_list(lexer, tok)
                        props[attr] = proplist
                        state = 3

                elif attr == VALUES:
                    if tok == EOL:
                        props[VALUES] = []
                        state = 6
                    else:
                        self.error = True
                        self.errors += 1
                        print("%sexpected EOL after values keyword" %
                              (lexer.error_leader()))

            elif state == 6:
                value = {}
                if tok == EOL:
                    state = 6

                elif props[TYPE] == INTEGER:
                    if tok == INTEGER:
                        value_type = INTEGER
                        state = 7
                    elif tok in KEYWORDS:
                        attr = tok
                        state = 4
                    else:
                        self.error = True
                        self.errors += 1
                        print("%sexpected 'integer' for integer properties" %
                              (lexer.error_leader()))

                elif props[TYPE] == STRING:
                    if tok == TOKEN:
                        value_type = TOKEN
                        state = 7
                    elif tok in KEYWORDS:
                        attr = tok
                        state = 4
                    else:
                        self.error = True
                        self.errors += 1
                        print("%sexpected 'token' for string properties" %
                              (lexer.error_leader()))

                elif props[TYPE] == REFERENCE:
                    if tok == REFERENCE:
                        value_type = REFERENCE
                        state = 7
                    elif tok in KEYWORDS:
                        attr = tok
                        state = 4
                    else:
                        self.error = True
                        self.errors += 1
                        print("%wanted 'reference' for reference properties" %
                              (lexer.error_leader()))

                elif props[TYPE] == PACKAGE:
                    if tok == SUBPACKAGE:
                        value_type = SUBPACKAGE
                        state = 12
                    elif tok in KEYWORDS:
                        attr = tok
                        state = 4
                    else:
                        self.error = True
                        self.errors += 1
                        print("%sexpected 'subpackage' for package properties" %
                              (lexer.error_leader()))

            elif state == 7:
                self.error, self.errors, state = \
                    self.__look_for_colon(attr, tok, lexer, self.errors, 8)
                
            elif state == 8:
                if tok != EOL:
                    value[value_type] = tok
                    state = 9
                else:
                    self.error = True
                    self.errors += 1
                    print("%sexpected a '%s' value, not EOL" %
                          (lexer.error_leader(), prop_type))
                
            elif state == 9:
                if tok == EOL:
                    state = 9
                elif tok == DESCRIPTION:
                    state = 10
                else:
                    self.error = True
                    self.errors += 1
                    print("%description is required for value" %
                          (lexer.error_leader()))

            elif state == 10:
                self.error, self.errors, state = \
                    self.__look_for_colon(attr, tok, lexer, self.errors, 11)

            elif state == 11:
                text = self.__get_raw_oneline(lexer, tok)
                value[DESCRIPTION] = text
                props[VALUES].append(value.copy())
                state = 6

            elif state == 12:
                self.error, self.errors, state = \
                    self.__look_for_colon(attr, tok, lexer, self.errors, 13)

            elif state == 13:
                if tok == LBRACE:
                    fields = []
                    state = 14
                else:
                    self.error = True
                    self.errors += 1
                    print("%expected '%s' to start package description" %
                          (lexer.error_leader(), LBRACE))

            elif state == 14:
                if tok == RBRACE:
                    value[value_type] = fields.copy()
                    state = 9
                elif tok == COMMA or tok == EOL:
                    state = 14
                elif tok == None:    # aka EOF
                    self.error = True
                    self.errors += 1
                    print("%expected '%s' to start package description" %
                          (lexer.error_leader(), LBRACE))
                else:
                    fields.append(tok)

            else:
                print("? internal error: state %d, token %s" % (state, tok))
                self.error = True
                self.errors += 1
                break

            if not self.error:
                tok = lexer.get_token()

        if prop not in self.properties.keys():
            self.properties[prop] = props.copy()

        for ii in self.properties.keys():
            if REQUIRES in self.properties[ii].keys():
                for jj in self.properties[ii][REQUIRES]:
                    if jj not in self.properties.keys():
                        print("? %s depends on %s, which does not exist" %
                              (ii, jj))
                        self.error = True
                        self.errors += 1
        
        print("File: %s" % (self.fname))
        print("%s lines read" % (self.lines))
        print("%s lines parsed, %d errors found" %
              (lexer.lineno - 1, self.errors))
        return self.error

    def get_property_locations(self):
        props = []
        for ii in self.properties.keys():
            path = os.path.join(os.sep, self.attrs[VENDOR], self.attrs[BUS],
                                self.attrs[DEVICE_ID], self.attrs[REVISION],
                                ii)
            props.append(path)
        return props

    def okay(self):
        return not self.error

    def property_set_name(self):
        return self.attrs[PROPERTY_SET]
