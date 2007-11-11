#!/usr/bin/python

import os,sys

if len(sys.argv) < 3:
	print "Usage: %s [number of nodes to launch] [network file]" % (sys.argv[0])
	sys.exit()

tolaunch = int(sys.argv[1])
network = sys.argv[2]
for node in range(tolaunch):
	os.system('xterm -geometry 100x30 -tn xterm-color -T "Node %d" -e ./ip_driver %d %s &' % (node, node, network))

