#!/usr/bin/env python3
#
# Copyright (c) 2106, Al Stone <ahs3@redhat.com>
#

import tools.env.config

def run(fnames):
    tools.env.config.dump()
    return

def usage_line():
    return "config                 \t=> list current config variables"

