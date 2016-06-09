#!/usr/bin/env python3
#
# Copyright (c) 2106, Al Stone <ahs3@redhat.com>
#

import os.path
import re
import sys

import tools.env.config
import tools.parser

APPROVED = "approved"
NEXTNUM=".nextnum"
PENDING = "pending"
ROOT = "root"

BASE_SETS = "00-base-sets"
SET_COPY = "01-orig-set"

class Database:

    def __init__(self, path=""):
        self.isopen = False
        self.path = path
        self.maintainers = tools.env.config.get_maintainers_path()
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

    def list_pending(self):
        plist = []
        path = os.path.join(self.path, PENDING)
        plist = os.listdir(path)
        plist.remove(NEXTNUM)
        return sorted(plist)

    def queued(self, qname):
        path = os.path.join(self.path, PENDING, qname)
        return os.path.exists(path)

    def pending_path(self, qname):
        return os.path.join(self.path, PENDING, qname)

    def approved_path(self, qname):
        return os.path.join(self.path, APPROVED, qname)

    def root_path(self, qname):
        return os.path.join(self.path, ROOT, qname)

    def list_approved(self):
        plist = []
        path = os.path.join(self.path, APPROVED)
        plist = os.listdir(path)
        plist.remove(NEXTNUM)
        return sorted(plist)

    def approver_okay(self, approver):
        found = False
        
        tmp = approver.split('<')
        if len(tmp) > 1:
            tmp = tmp[1].split('>')
        else:
            return False
        apr = tmp[0]
        if not apr:
            return False

        f = open(self.maintainers, 'r')
        f.seek(0)
        alist = f.readlines()
        prog = re.compile(apr)
        for ii in alist:
            if ii[0:2] != "M:":
                continue
            if prog.search(ii):
                found = True
                break

        f.close()
        return found

    def add(self, p):
        # p is a tools.parser.Parser object

        # build the directory tree for the property set
        pdir = os.path.join(self.path, ROOT, p.get_set_location()[1:])
        if not os.path.exists(pdir):
            os.makedirs(pdir)

        # add in the file of base property sets to inherit from
        bsets = open(os.path.join(pdir, BASE_SETS), 'w')
        bases = p.get_base_sets()
        for ii in bases:
            bsets.write("%s\n" %(ii))
        bsets.close()

        # write all the property descriptions
        props = p.get_properties()
        for ii in props:
            f = open(os.path.join(pdir, ii), 'w')
            p.write_property(ii, f)
            print("   %s has been added" % (ii))
            f.close()

        # copy the original text to the directory tree
        cp = open(os.path.join(pdir, SET_COPY), "w")
        cp.write(p.get_source())
        cp.close()

        return

