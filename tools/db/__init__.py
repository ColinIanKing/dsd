#!/usr/bin/env python3
#
# Copyright (c) 2106, Al Stone <ahs3@redhat.com>
#

import os.path
import sys

import tools.env.config

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

