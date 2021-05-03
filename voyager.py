# voyager's prototype

from scripts.utils import *

from random import sample, random, seed
import threading
import time

from ryu.base import app_manager
from ryu.controller import ofp_event
from ryu.controller.handler import CONFIG_DISPATCHER, MAIN_DISPATCHER
from ryu.controller.handler import set_ev_cls
from ryu.ofproto import ofproto_v1_3
from ryu.ofproto import oxm_fields
from ryu.lib import type_desc
from ryu.lib.packet import packet, ethernet, ipv4, ipv6
from ryu.lib import addrconv
from ryu.lib.packet import ether_types

class Voyager(app_manager.RyuApp):
    OFP_VERSIONS = [ofproto_v1_3.OFP_VERSION]
    
    # add a custom field 'voyager' to the protocol (using ONF experimenter id for now)
    ofproto_v1_3.oxm_types += [oxm_fields.ONFExperimenter('voyager', 77, type_desc.IPv6Addr)]
    oxm_fields.generate('ryu.ofproto.ofproto_v1_3')

    def __init__(self, *args, **kwargs):
        super(Voyager, self).__init__(*args, **kwargs)
        
        # parse config
        config = parseConfig('default.cfg')
        self.toponame = config['toponame']
        self.errlist = [float(e) for e in config['err'].split('|')]
        self.err = self.errlist.pop(0)
        self.timeout = float(config['timeout'])
        self.custom = bool(int(config['custom']))
        
        # parse topology and headers
        self.neighbors, self.rules, self.flows = parseTopo(self.toponame)
        self.assignments, self.per_rule_hdrs, self.per_path_hdrs = parseHeaders(self.toponame)
        self.dps = {}

        self.ready = False
    
    @set_ev_cls(ofp_event.EventOFPSwitchFeatures, CONFIG_DISPATCHER)
    def switch_features_handler(self, ev):
        datapath = ev.msg.datapath

        # save datapath
        dpid = format(datapath.id, 'd').zfill(16)
        self.dps[dpid] = datapath
        
        # a switch may send switch_feature message more than once (triggered by what?)
        # use 'self.ready' to prevent from repeated executions
        if(len(self.dps) == len(self.neighbors)):
            if not self.ready:
                print("[ ] switches are ready.", len(self.dps))
                self.ready = True
                self.start_everything()
            else:
                print("[!] repeat connection. dpid =", dpid)
                pass

    def start_everything(self):
        print("[ ] start everything.", self.err)
        self.tpid = 1000        # global test packet id
        self.tpcount = [0, 0]   # count test packet
        self.latepacket = -1    # time of arrival (for the last late packet)
        self.launch_time = {}   # time of launch (for all packets)
        
        self.flag_2nd = False   # if the 2nd round test has been launched

        self.passed = set()     # passed rules
        self.faults = set()     # faulty rules identified
        
        # attack simulation - part 1
        seed("voyager")
        self.err_n = int(self.err * (len(self.rules)))
        self.attacked = set(sample(self.rules.keys(), self.err_n))
        
        for dpid in self.dps:
            # install flow entries (both non-report and report rules)
            self.install_rules(self.dps[dpid])
        
        time.sleep(1)

        self.start_time1 = time.time()
        self.prepare_test_pkts()
        self.time1 = time.time() - self.start_time1

        # fault localization. launch test packets in parallel
        # TODO: trigger it periodcally for deployment
        self.start_time2 = time.time()
        self.launch_1st_round_test()

    def install_rules(self, datapath):
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser

        msid = datapath.id  # msid == dpid
        
        # install non-report rules
        for rid in self.flows[msid]:
            # attack simulation - part 2
            if rid in self.attacked: continue
            
            # install rules not attacked 
            rule = self.rules[rid]
            priority = rule.priority
            kwargs = dict(in_port=rule.in_port, eth_type=ether_types.ETH_TYPE_IP)
            kwargs['ipv4_dst'] = (rule.ip, rule.mask)
            match = parser.OFPMatch(**kwargs) 
            actions = [parser.OFPActionOutput(rule.out_port)]
            self.add_flow(datapath, priority, match, actions)

        # install report rules
        priority = 0xffff
        
        if self.custom:
            kwargs = dict(eth_type=ether_types.ETH_TYPE_IP)
            kwargs['ip_proto'] = 192
            kwargs['voyager'] = self.compose_report_hdr(msid)
        else:
            kwargs = dict(eth_type=ether_types.ETH_TYPE_IP)
            kwargs['ipv4_src'] = self.compose_report_hdr(msid)

        match = parser.OFPMatch(**kwargs)
        actions = [parser.OFPActionOutput(ofproto.OFPP_CONTROLLER, ofproto.OFPCML_NO_BUFFER)]
        self.add_flow(datapath, priority, match, actions)

    def compose_report_hdr(self, msid):
        masklen = self.assignments[-1]
        maskbit, value = self.assignments[msid]

        report_hdr_value = '0' * (masklen - maskbit - 1) + str(value) + '0' * maskbit
        report_hdr_mask = '0' * (masklen - maskbit - 1) + '1' + '0' * maskbit
        
        if self.custom:
            return ipv6fmt(report_hdr_value), ipv6fmt(report_hdr_mask)
        else:
            return ipfmt(report_hdr_value), ipfmt(report_hdr_mask) 

    def prepare_test_pkts(self):
        self.tests = set()      # (tpid)
        self.tested_rules = {}  # tested rules per-path. tpid -> (rid)
        self.test_expected = {} # tpid -> dpid. expected dpid
        self.test_pkts1 = {}    # 1st round test packets. tpid -> (dpid, out)
        self.test_single = {}   # for per-rule tests in the 2nd round. rid -> (tpid, dpid, out)
        self.test_pkts2 = {}    # 2nd round test packets. tpid -> (dpid, out)
        self.test_timers = {}   # failed test handler. tpid -> class timer

        # for each tested path (multiple or single rule)
        for rule_path in self.per_path_hdrs:
            tpid, dpid, out = self.prepare_test_pkt(rule_path)
            self.test_pkts1[tpid] = tuple([dpid, out])

        # for each single rule
        for rid in self.rules:
            rule_path = tuple([rid])
            tpid, dpid, out = self.prepare_test_pkt(rule_path)
            self.test_single[rid] = tuple([tpid, dpid, out])
            
    def prepare_test_pkt(self, rule_path):
        tpid = self.tpid

        self.tested_rules[tpid] = rule_path
        self.test_expected[tpid] = self.rules[rule_path[-1]].out_port

        start_rid = self.tested_rules[tpid][0]
        start_msid = self.rules[start_rid].msid

        # determine the required packet header and test header
        if rule_path in self.per_path_hdrs:
            # pre-calculated
            pkt_hdr, test_hdr = self.per_path_hdrs[rule_path]
        else:
            # dynamic (for newly-installed or suspicious rules)
            # it should be triggered exactly by one rule as a path
            # TODO: support general packet header
            pkt_hdr = self.rules[start_rid].hdr 
            test_hdr = self.per_rule_hdrs[start_msid]
        
        voyager_payload = self.compose_voyager_payload(tpid)
        pkt = self.compose_test_pkt(pkt_hdr, test_hdr, voyager_payload)

        # encapsulate the packet out message
        dpid = format(start_msid, 'd').zfill(16)
        datapath = self.dps[dpid]
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser
        in_port = self.rules[start_rid].in_port
        actions = [parser.OFPActionOutput(ofproto.OFPP_TABLE)] # submit to the first table
        
        out = parser.OFPPacketOut(datapath=datapath, buffer_id=ofproto.OFP_NO_BUFFER,
                                  in_port=in_port, actions=actions, data=pkt.data)

        # print("packet prepared. tpid=%d expected_dpid=%d start_msid=%d in_port=%d rule_path=" % (tpid, self.test_expected[tpid], start_msid, in_port), rule_path)
        self.tpid += 1
        return tpid, dpid, out
    
    def compose_test_pkt(self, pkt_hdr, test_hdr, voyager_payload):
        pkt = packet.Packet()
        pkt.add_protocol(ethernet.ethernet(ethertype=ether_types.ETH_TYPE_IP))
        
        final_pkt_hdr = ipfmt(pkt_hdr.replace('x', '0'))
        if self.custom:
            pkt.add_protocol(ipv4.ipv4(dst=final_pkt_hdr, proto=192))
            final_test_hdr = ipv6fmt(test_hdr.replace('x', '0'))
            pkt.add_protocol(addrconv.ipv6.text_to_bin(final_test_hdr))
        else:
            final_test_hdr = ipfmt(test_hdr.replace('x', '0'))
            pkt.add_protocol(ipv4.ipv4(dst=final_pkt_hdr, src=final_test_hdr))
        
        pkt.add_protocol(bytes(voyager_payload, encoding='utf-8'))
        pkt.serialize()
        # print(pkt)
        return pkt
    
    def compose_voyager_payload(self, tpid):
        magic = 'voyager#'
        return magic + str(tpid)
    
    def launch_1st_round_test(self):
        print("[ ] start the 1st round.", len(self.test_pkts1))
        for tpid in self.test_pkts1:
            self.tests.add(tpid)
        
        for tpid in self.test_pkts1:
            self.tpcount[0] += 1
            dpid, out = self.test_pkts1[tpid]
            self.launch_test(tpid, dpid, out)

    def launch_2nd_round_test(self):
        print("[ ] start the 2nd round.", len(self.test_pkts2))
        for tpid in self.test_pkts2:
            self.tests.add(tpid)
        
        for tpid in self.test_pkts2:
            self.tpcount[1] += 1
            dpid, out = self.test_pkts2[tpid]
            self.launch_test(tpid, dpid, out)

    def launch_test(self, tpid, dpid, out):
        # print("[ ] test launched:", tpid)
        # register a timer for the test (canceled in packet-in)
        if self.is_negative_test(tpid):
            # pass a negative test later
            self.test_timers[tpid] = threading.Timer(0.5, self.handle_passed_test, [tpid])
        else: 
            # launch per-rule test later
            timeout = self.timeout
            self.test_timers[tpid] = threading.Timer(timeout, self.handle_failed_test, [tpid])
        
        self.launch_time[tpid] = time.time()
        self.test_timers[tpid].start()
        
        self.dps[dpid].send_msg(out)

    @set_ev_cls(ofp_event.EventOFPPacketIn, MAIN_DISPATCHER)
    def _packet_in_handler(self, ev):
        msg = ev.msg
        pkt = packet.Packet(msg.data)
        dpid = msg.datapath.id

        protocols = self.get_protocols(pkt)
        if self.custom:
            voyager_payload = str(protocols['payload'][16:], encoding='utf-8')
        else:
            voyager_payload = str(protocols['payload'], encoding='utf-8')
        tpid = int(voyager_payload.split('voyager#')[1])
        
        # the assertion will be violated if timer interval is too short
        # assert tpid in self.tests
        if tpid not in self.tests:
            self.latepacket = time.time() - self.start_time2
            print("[!] late packet. tpid=%d, len=%d, span=%.2f" % (tpid, len(self.tested_rules[tpid]), time.time() - self.launch_time[tpid]))
            return
        
        expected = self.test_expected[tpid]
        # print("[ ] packet in. tpid=%d dpid=%d expected=%d start_msid=%d rules=" % (tpid, dpid, expected, self.rules[self.tested_rules[tpid][0]].msid), self.tested_rules[tpid])
        if dpid != expected:
            print("[!] unexpected reporter. tpid=%d dpid=%d expected=%d targets =" % (tpid, dpid, expected), [self.rules[rid].msid for rid in self.tested_rules[tpid]], "rules =", self.tested_rules[tpid])
            # self.handle_failed_test(tpid)
            return

        # a negative test failed
        if self.is_negative_test(tpid):
            print("[!] negative test. tpid =", tpid)
            self.handle_failed_test(tpid)
            return

        # a positive test passed
        self.handle_passed_test(tpid)
        
    def handle_passed_test(self, tpid):
        # print("[ ] test passed:", tpid)
        self.test_timers[tpid].cancel()     # cancel the timer
        self.passed |= set(self.tested_rules[tpid])
        self.complete_test(tpid)

    def handle_failed_test(self, tpid):
        # print("[ ] test failed:", tpid)
        if tpid not in self.tests:
            print("[x] passed before failed. tpid =", tpid)
            return

        suspects = self.tested_rules[tpid]
        if len(suspects) > 1:
            # launch per-rule test. Rule paths are non-disjointed, so excluding
            # identified faults can reduce the volume of test traffic in this
            # second round.
            for rid in set(suspects) - self.faults - self.passed:
                _tpid, dpid, out = self.test_single[rid]
                self.test_pkts2[_tpid] = tuple([dpid, out])
        else: 
            self.faults.add(suspects[0])
        
        self.complete_test(tpid)
    
    def complete_test(self, tpid):
        self.tests.remove(tpid)
        
        # some tests are still in-flight
        if len(self.tests) > 0:
            return
        
        # the 1st round tests completed, start the 2nd round
        if not self.flag_2nd and len(self.test_pkts2) > 0:
            self.flag_2nd = True
            self.launch_2nd_round_test()
            return
        
        # all tests completed
        if len(self.tests) == 0:
            # record test results
            self.time2 = time.time() - self.start_time2
            total_p = len(self.attacked)
            total_n = len(self.rules.keys()) - len(self.attacked)
            fp = len(self.faults - self.attacked)
            fn = len(self.attacked - self.faults)
            fpr = fp / total_n if total_n > 0 else 0.0
            fnr = fn / total_p if total_p > 0 else 0.0

            # two equal sets are expected
            print("[*] test report for an error rate of", self.err)
            print("    fp, fpr:", fp, fpr)
            if fp: print("[x] fp[:10]:", sorted(list(self.faults - self.attacked))[:10])
            print("    fn, fnr:", fn, fnr)
            if fn: print("[x] fn[:10]:", sorted(list(self.attacked - self.faults))[:10])
            print("    tpcount:", self.tpcount)
            if self.latepacket != -1: print("[x] arrival time of the last late packet:", self.latepacket)
            print("    time:", self.time1, self.time2)

            # start with the next error rate
            if len(self.errlist) > 0:
                self.err = self.errlist.pop(0)

                for dpid in self.dps:
                    self.reset_flow_table(self.dps[dpid])
                
                time.sleep(1)
                self.start_everything()
            else:
                print("[*] Completed")

    def is_negative_test(self, tpid):
        end_rid = self.tested_rules[tpid][-1]
        return self.rules[end_rid].out_port == -1

    def add_flow(self, datapath, priority, match, actions, buffer_id=None):
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser
        inst = [parser.OFPInstructionActions(ofproto.OFPIT_APPLY_ACTIONS, actions)]
        kwargs = dict(datapath=datapath, priority=priority, match=match,
                      instructions=inst, command=ofproto.OFPFC_ADD)
        if buffer_id:
            kwargs['buffer_id'] = buffer_id
        
        mod = parser.OFPFlowMod(**kwargs)
        datapath.send_msg(mod)

    def reset_flow_table(self, datapath):
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser
        
        # remove all flow entries
        # uncommented parameters are indispensable to clear
        kwargs = dict(datapath=datapath,
                      command=ofproto.OFPFC_DELETE,
                      # priority=1,
                      # buffer_id=ofproto.OFP_NO_BUFFER,
                      out_port=ofproto.OFPP_ANY,
                      out_group=ofproto.OFPG_ANY,
                      # flags=ofproto.OFPFF_SEND_FLOW_REM,
                      # match=parser.OFPMatch(),
                      # instructions=[]
                      ) 
        mod = parser.OFPFlowMod(**kwargs)
        datapath.send_msg(mod)

    def get_protocols(self, pkt):
        protocols = {}
        for p in pkt:
            if hasattr(p, 'protocol_name'):
                protocols[p.protocol_name] = p
            else:
                protocols['payload'] = p
        return protocols
