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

        # make sure the properties don't already exist
        props = p.get_property_locations()
        for jj in props:
            if db.property_exists(jj):
                print("? property already exists:" % (jj))
                break

        if p.okay():
            print('queueing property-set "%s" ...' %
                  (p.property_set_name()), end='')
            path = db.new_queue(p.property_set_name(), ii)
            if path:
                print(" done.")
            else:
                print(" FAILED.")
            print('queued as: "%s"' % (path))
        else:
            print('? errors in property-set "%s", not queued' %
                  (p.property_set_name()))

    db.close()
    return

def usage_line():
    return "queue <files>...      \t=> queue files for review/inclusion"

