#!/usr/bin/env python3
#
# Copyright (c) 2106, Al Stone <ahs3@redhat.com>
#

import sys

import tools.cmds.approve
import tools.cmds.check
import tools.cmds.config
import tools.cmds.init
import tools.cmds.queue
import tools.cmds.usage
import tools.env.config

__cmd = ''
__args = []


def get():
    global __cmd, __args

    if len(sys.argv) > 1:
        __cmd = sys.argv[1]
    if len(sys.argv) > 2:
        __args = sys.argv[2:]
    return

def print_line():
    global __cmd, __args

    if __cmd == '':
        print("? no command given")
        return

    print("command: %s %s" % (__cmd, " ".join(__args)))

    return

def run():
    get()

    if __cmd == "" or __cmd == "help" or __cmd == "?":
        tools.cmds.usage.usage()

    elif __cmd == "check":
        if len (__args) > 0:
            tools.cmds.check.run(__args)
        else:
            print("? check command requires a file to check")

    elif __cmd == "init":
        db = ''
        if len(__args) > 0:
            db = __args[0]
        else:
            db = tools.env.config.get_dbpath()
        tools.cmds.init.run(db)

    elif __cmd == "queue":
        if len (__args) > 0:
            tools.cmds.queue.run(__args)
        else:
            print("? queue command requires a file to put in the queue")

    elif __cmd == "approve":
        if len (__args) > 0:
            tools.cmds.approve.run(__args)
        else:
            print("? approve command requires a queued name")

    elif __cmd == "config":
        tools.cmds.config.run(__args)

    else:
        print("? no such command: %s" % (__cmd))

    return
