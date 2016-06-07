#!/usr/bin/env python3
#
# Copyright (c) 2106, Al Stone <ahs3@redhat.com>
#

import os.path
import sys

PENDING = "pending"
APPROVED = "approved"
ROOT = "root"
NEXTNUM=".nextnum"

def run(dbname):
    print("creating db: %s" % (dbname))
    if os.path.isdir(dbname):
        print("! db directory already exists")
    else:
        os.makedirs(dbname)

    subd = os.path.join(dbname, ROOT)
    if not os.path.isdir(subd):
        os.makedirs(subd)

    for ii in [PENDING, APPROVED]:
        subd = os.path.join(dbname, ii)
        if not os.path.isdir(subd):
            os.makedirs(subd)
        nextnum = 0
        with open(os.path.join(subd, NEXTNUM), "w") as f:
            f.write("%s\n" % (str(nextnum)))
            f.close()

    return

def usage_line():
    return "init [<dbpath>]\t\t=> initialize a db"

