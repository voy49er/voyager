#!/usr/bin/python3

import sys

from utils import *

from mininet.cli import CLI
from mininet.net import Mininet
from mininet.node import RemoteController
from mininet.topo import Topo
from mininet.util import dumpNodeConnections

class ATopo(Topo):
    def __init__(self):
        Topo.__init__(self)
        self.neighbors, _, _ = parseTopo(topofile)
        self.settled = []   # switches that have added all neighboring links

        # add all switches
        for sid in self.neighbors:
            self.addSwitch(self.label(sid), dpid=self.dpid(sid), protocols='OpenFlow13')

        # add all links
        for s1 in self.neighbors:
            for s2 in self.neighbors[s1]:
                if s2 not in self.settled:
                    self.addLink(self.label(s1), self.label(s2), port1=msid(s2), port2=msid(s1))
            self.settled.append(s1)
    
    def label(self, sid):
        return 's' + str(msid(sid))
    
    def dpid(self, sid):
        # dpid cannot be 0 in mininet
        return format(msid(sid), 'x').zfill(16)

def mininet():
    net = Mininet(topo=ATopo(), controller=None)
    net.addController('c0', controller=RemoteController, ip='127.0.0.1', port=6653)
    net.start()
    
    dumpNodeConnections(net.controllers + net.switches)

    CLI(net)
    net.stop()

if __name__ == '__main__':
    assert len(sys.argv) == 2
    topofile = sys.argv[1]
    mininet()

