#!/usr/bin/python

import os,sys

tolaunch = int(sys.argv[1])
for x in range(tolaunch):
	node = str(x)
	os.system("xterm -tn xterm-color -T \"Node " + node + "\" -e ./ip_driver " + node + " networks/netconfig-bigp2p &")

