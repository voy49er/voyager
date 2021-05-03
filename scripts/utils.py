import os
curpath = os.path.split(os.path.realpath(__file__))[0]
path_config = curpath + '/../config/'
path_topo = curpath + '/../data/topo/'
path_store = curpath + '/../data/store/'

port_host = 1000

class Rule():
    def __init__(self, sid, rid, prefix, in_port, out_port, priority):
        self.msid = msid(sid)
        self.rid = rid
        self.prefix = prefix
        self.ip, self.mask, self.hdr = parsePrefix(prefix)
        self.in_port = msid(in_port)
        self.out_port = msid(out_port)
        self.priority = priority

def parseTopo(toponame):
    neighbors = {}  # switch graph. sid -> [sid]
    rules = {}      # rule set. rid -> [class Rule]
    flows = {}      # rules associated with switches. sid -> [rid] 
    with open(path_topo + toponame, 'r') as f:
        # get 'neighbors'
        ns = int(f.readline())
        for _ in range(ns):
            sids = [int(x) for x in f.readline().split()]
            neighbors[sids[0]] = sids[1:]
        
        for _ in range(ns):
            # get 'rules' and 'flows' for each sid
            sid, nr = [int(x) for x in f.readline().split()]
            flows[msid(sid)] = []
            for _ in range(nr):
                xs = f.readline().split()
                rid = int(xs[0])
                prefix = xs[1]
                in_port = int(xs[2])
                out_port = int(xs[3])
                priority = int(xs[4])
                if out_port != port_host:
                    rules[rid] = Rule(sid, rid, prefix, in_port, out_port, priority)
                    flows[msid(sid)].append(rid)
    
    return neighbors, rules, flows

def parseHeaders(toponame):
    assignments = {}    # switch's report header. msid -> (maskbit, value)
    per_rule_hdrs = {}  # single. msid -> testheader
    per_path_hdrs = {}  # multiple (likely single). (rid) -> (pkt hdr, test hdr)
    
    toponame = toponame.replace(".topo", "")
    with open(path_store + toponame + '.switch.store', 'r') as f:
        masklen = int(f.readline())
        assignments[-1] = masklen
        for line in f.readlines():
            xs = line.split()
            sid = int(xs[0])
            maskbit = int(xs[1])
            value = int(xs[2])
            test_hdr = xs[3]
            assignments[msid(sid)] = (maskbit, value)
            per_rule_hdrs[msid(sid)] = test_hdr
    
    with open(path_store + toponame + '.path.store', 'r') as f:
        for line in f.readlines():
            xs = line.split()
            rule_path = tuple(int(x) for x in xs[0].split('|'))
            pkt_hdr = xs[1]
            test_hdr = xs[2]
            per_path_hdrs[rule_path] = (pkt_hdr, test_hdr)
    
    return assignments, per_rule_hdrs, per_path_hdrs

def parseConfig(cfgname):
    config = {}
    with open(path_config + cfgname, 'r') as f:
        for line in f:
            attr, value = line.strip().split('=')
            config[attr] = value
    
    return config

def parsePrefix(prefix):
    # parse prefix 192.168.1.0/24 as
    # - ip = 192.168.1.0 and
    # - mask = 255.255.255.0
    if prefix.find('/') == -1:
        ip = prefix
        mask = '255.255.255.255'
        
        binstr = ''.join([format(int(x), 'b').zfill(8) for x in ip.split('.')])
        hdr = binstr
    else:
        ip = prefix[:prefix.find('/')]
        mask_len = int(prefix[prefix.find('/') + 1:])
        mask = ipfmt('1' * mask_len + '0' * (32 - mask_len))
        
        binstr = ''.join([format(int(x), 'b').zfill(8) for x in ip.split('.')])
        hdr = binstr[:mask_len] + 'x' * (32 - mask_len)
    
    return ip, mask, hdr

def ipfmt(binstr):
    assert len(binstr) <= 32
    zbinstr = (32 - len(binstr)) * '0' + binstr
    intstr = [int(zbinstr[i*8:i*8+8], 2) for i in range(4)]
    ipstr = '.'.join([str(x) for x in intstr])
    return ipstr

def ipv6fmt(binstr):
    assert len(binstr) <= 128
    zbinstr = (128 - len(binstr)) * '0' + binstr
    hexstr = [hex(int(zbinstr[i*16:i*16+16], 2)).replace('0x', '').zfill(4) for i in range(8)]
    ipv6str = ':'.join([str(x) for x in hexstr])
    return ipv6str

def msid(sid):
    # use 'msid' instead of 'sid' in mininet
    # something wrong with a label 's0' and interfaces 'eth0' even if 'dpid' is non-zero
    # mapping 'sid' to 'sid' + 1 does help, but why?
    return sid + 1
