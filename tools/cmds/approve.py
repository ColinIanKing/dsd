#!/usr/bin/env python3
#
# Copyright (c) 2106, Al Stone <ahs3@redhat.com>
#

import os

import tools.db
import tools.env.config

def run(qnames):
    db = tools.db.Database()
    db.open()
    for ii in qnames:
        if ii == "--list":
            alist = db.list_approved()
            if alist:
                for jj in alist:
                    print('%s' % (jj))
            continue

        if db.queued(ii):
            print('found queued set %s' % (ii))
            p = tools.parser.Parser(db.pending_path(ii))
            if p.okay():
                approvals = p.get_acks()
                count = 0
                for jj in approvals:
                    print("acked-by: %s ... " % (jj), end='')
                    if db.approver_okay(jj):
                        print("OK")
                        count += 1
                    else:
                        print("FAILED")

                if count > 0:
                    print("adding %s to database" % (ii))
                    db.add(p)
                    # move it to the approved list
                    os.rename(db.pending_path(ii), db.approved_path(ii))
                else:
                    print("%s needs ACKs from known approvers to be added" % (ii))

    db.close()
    return

def usage_line():
    return "approve <queued>...   \t=> approve sets for inclusion"

