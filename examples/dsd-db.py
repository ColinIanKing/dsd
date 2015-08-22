#!/usr/bin/env python
#
# If one uses this script in a .forward or mail alias like this:
#
#	| /some-dir/dsd-db.py
#
# and sets the values of DBDIR (the directory containing the dsd database)
# and DBNAME (the name of the database), and then makes sure that SMTP is
# working for the alias or the .forward, this script will read in emails
# with the dsd commands:
#
#	lookup <name>
#
# or:
#
#	list { devs | props | all }
#
# with 'all' the default if none is given.
#
# Those commands will then be run against DBNAME and the results returned
# to whoever sent the original email.
#
# Your email environment may require other changes, so YMMV.
#

import os
import sys
import subprocess
import email.parser
from email.mime.text import MIMEText
import smtplib
import tempfile

DBDIR = '~/dsd'
DBNAME = 'db'

def run_cmd(cmd):
	cur = os.getcwd()
	os.chdir(DBDIR)

	args = cmd.split(' ')
	q = './dsd ' + args[0] + ' ' + DBNAME + ' ' + ' '.join(args[1:])
	output = subprocess.check_output(q.split(' '))
	print output

	os.chdir(cur)

#-- main starts here

# get the message
raw_mail = sys.stdin.readlines()
mail = "".join(raw_mail)

# parse it in order to build a reply
message = email.message_from_string(mail)
cmds = message.get_payload().split('\n')

# build up the reply message
fp = tempfile.TemporaryFile()
tmp = sys.stdout
sys.stdout = fp

for ii in cmds:
	args = ii.split(" ")
	for jj in range(0, len(args)):
		args[jj] = args[jj].replace('\r', '')
	cmd = ' '.join(args)
	if len(args) > 1:
		if args[0] == 'lookup' or args[0] == 'list':
			print "\n>\n> %s\n>\n" % cmd
			run_cmd(cmd)
		else:
			print "\n>\n> end of commands\n>\n"
			break

sys.stdout = tmp
fp.seek(0)
outmsg = fp.readlines()
fp.close()

# send the reply
s = smtplib.SMTP('localhost')
s.sendmail('do-not-reply@somewhere.else', message['from'], ''.join(outmsg))
s.quit()

