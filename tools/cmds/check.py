#!/usr/bin/env python3
#
# Copyright (c) 2106, Al Stone <ahs3@redhat.com>
#

import tools.parser
import tools.db

def run(fnames):
    db = tools.db.Database()
    db.open()
    for ii in fnames:
        p = tools.parser.Parser(ii)
        #p.dump()
        p.print_result()

        # make sure the properties don't already exist
        props = p.get_property_locations()
        for ii in props:
            if db.property_exists(ii):
                print("? property already exists:" % (ii))

    db.close()
    return

def usage_line():
    return "check <files>...      \t=> check files for correctness"

