#!/usr/bin/env python3
#
# Copyright (c) 2106, Al Stone <ahs3@redhat.com>
#

import tools.parser

def run(fnames):
    for ii in fnames:
        p = tools.parser.Parser(ii)
        p.dump()
    return

def usage_line():
    return "check <files>...      \t=> check files for correctness"

