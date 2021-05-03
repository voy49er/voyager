#!/usr/bin/python3

port_host = 1000

import sys
import networkx as nx

import time
start_time = time.time()

import os
cur = os.path.split(os.path.realpath(__file__))[0]

inpath = cur + "/zoo"
outpath = cur + "/output"

from collections import Counter

def build_rules(graph, kspaths):
    rules = {}
    pathset = []

    rid = 10000
    for src in graph.nodes():
        rules[src] = []

        priority = 8000 
        for dst in graph.nodes():
            zbinstr16 = format(int(dst), 'b').zfill(16)
            assert len(zbinstr16) == 16
            prefix = "10.%d.%d.0/24" % (int(zbinstr16[:8], 2), int(zbinstr16[8:], 2))

            if dst != src:
                if(len(kspaths[src][dst]) == 0): continue   # unreachable
                spath = kspaths[src][dst][0]
                out_port = spath[1]
            else:
                out_port = port_host
            
            for in_port in list(graph.neighbors(src)) + [port_host]:
                if in_port == out_port: continue

                rule = [rid, prefix, in_port, out_port, priority]
                rules[src].append(rule)
                rid += 1
                priority += 1

    return rules, pathset

def ksp(graph, k):
    kspaths = {}    # k shortest paths
    for src in graph.nodes():
        kspaths[src] = {}
        for dst in graph.nodes():
            if dst != src:
                # get paths in order from shortest to longest
                kspaths[src][dst] = []
                try:
                    sp_generator = nx.shortest_simple_paths(graph, src, dst)
                    
                    # save the shortest k paths
                    for idx, spath in enumerate(sp_generator):
                        kspaths[src][dst].append(spath)
                        if idx == k - 1:
                            break
                
                except Exception:
                    pass

    return kspaths

def handle(fname):
    # parse
    f = open(os.path.join(inpath, fname), 'r')
    
    switches = []
    links = []
    
    for line in f:
        if line.startswith("    id") and line[7] != "\"" :
            sid = line.split()[1]
            switches.append(sid)
        if line.startswith("    source"):
            s1 = line.split()[1]
        if line.startswith("    target"):
            s2 = line.split()[1]
            links.append((s1, s2))
    
    # build
    graph = nx.Graph() 
    graph.add_nodes_from(switches)
    graph.add_edges_from(links)

    # setup
    k = 1
    kspaths = ksp(graph, k)
    rules, pathset = build_rules(graph, kspaths)
    
    # count
    n_v = len(graph.nodes)
    n_e = len(graph.edges)
    n_r = 0
    for s in rules:
        n_r = n_r + len(rules[s])
    
    # output
    
    # voyager topo
    newfname = "s%d.r%d.%s" % (n_v, n_r, fname.replace(".gml", ".topo"))
    with open(os.path.join(outpath, newfname), 'w') as f:
        # - switch graph
        f.write("%d\n" % (len(switches)))
        for s1 in graph:
            f.write(str(s1))
            for s2 in graph[s1]:
                f.write(" " + str(s2))
            f.write("\n")
    
        # - rules
        for s in graph:
            if s not in rules:
                f.write("%s 0\n" % s)
                continue

            f.write("%s %d\n" % (s, len(rules[s])))
            for r in rules[s]:
                f.write("%d %s %s %s %d\n" % tuple(r))
    
    print("[ ] [%s] [e=%d] [t=%f]" % (newfname, n_e, time.time()-start_time))

if __name__ == "__main__":
    inpath = cur + "/" + sys.argv[1]
    for root, dirs, files in os.walk(inpath):
        for fname in files:
            if fname in ["LambdaNet.gml", "Columbus.gml", "Syringa.gml", "RedBestel.gml", "Deltacom.gml"]:
                start_time = time.time()
                handle(fname)
