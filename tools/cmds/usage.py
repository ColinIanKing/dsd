#!/usr/bin/env python3
#
# Copyright (c) 2106, Al Stone <ahs3@redhat.com>
#

import tools.env
import tools.cmds.init
import tools.cmds.usage

def usage():
    print
    tools.env.print_version()

    print("usage: %s <command> [ <options> ...]" % (tools.env.program()))
    print("where <command> can be:")

    print("\t%s" % (tools.cmds.check.usage_line()))   # "check" command
    print("\t%s" % (tools.cmds.usage.usage_line()))     # "help" command
    print("\t%s" % (tools.cmds.init.usage_line()))      # "init" command

    return

def usage_line():
    return "help\t\t\t=> return this info"

