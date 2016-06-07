#!/usr/bin/env python3
#
# Copyright (c) 2106, Al Stone <ahs3@redhat.com>
#

import os.path
import sys

import tools.env.config

PENDING = "pending"
APPROVED = "approved"
ROOT = "root"
NEXTNUM=".nextnum"

class Database:

    def __init__(self, path=""):
        self.isopen = False
        self.path = path
        return

    def open(self, path=""):
        if path:
            self.path = path
        if not self.path:
            self.path = tools.env.config.get_dbpath()
        if not os.path.exists(self.path):
            print("? no such database: %s" % (self.path))
            sys.exit(1)

        # so, we're using a directory tree; not much to do, really
        self.isopen = True
        return

    def close(self):
        self.isopen = False
        return

    def property_exists(self, path):
        p = os.path.join(self.path, path)
        return os.path.exists(os.path.normpath(p))

    def get_next_num(self, dname):
        result = 0
        npath = os.path.join(self.path, dname, NEXTNUM)
        if os.path.exists(npath):
            f = open(npath, "r+")
            result = int(f.read())
            f.close()
            result += 1
            f = open(npath, "w")
            f.write("%d\n" % (result))
            f.close()
        return result

    def new_queue(self, set_name, fname):
        path = ''
        if os.path.exists(fname):
            sname = set_name.replace(" ", "-")
            num = self.get_next_num(PENDING)
            path = "%06d-%s" % (num, sname)

            i = open(fname, "r")
            o = open(os.path.join(self.path, PENDING, path), "w")
            o.write(i.read())
            i.close()
            o.close()
        return path

