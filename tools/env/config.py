#!/usr/bin/env python3
#
# Copyright (c) 2106, Al Stone <ahs3@redhat.com>
#

import os.path
import configparser

defaultrc="/etc/dsdrc"
userrc="$HOME/.dsdrc"

config = configparser.ConfigParser()
config['database'] = { "path" : "./db"
                     }
config['user'] = { "name" : "",
                   "email" : "",
                 }

def readrc():
    global config

    for ii in [defaultrc, userrc]:
        fpath = os.path.expandvars(ii)
        if os.path.exists(fpath):
            with open(fpath, "r") as cfile:
                config.read_file(cfile)

    config.set("database", "path", 
               os.path.expandvars(config.get("database", "path")))

    return

def dump():
    global config

    for ii in config.sections():
        print("[%s]" % (ii))

        for jj in config.options(ii):
            print("\t%s = %s" % (jj, config[ii][jj]))

    return

def get_dbpath():
    global config

    return config.get("database", "path")

