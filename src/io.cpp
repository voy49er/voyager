#include "io.hpp"

void IO::LoadTopo(const string &name, SwitchGraph &sg, RuleGraph &rg)
{
    ifstream fin("../data/topo/" + name);
    if(!fin) {
        throw "topo file does not exist.";
    }
    
    // build switch graph
    int ns;
    fin >> ns;
    
    int sid, neighbor;
    for(int idx = 0; idx < ns; idx++) {
        fin >> sid;
        sg[sid] = SwitchNode(sid, idx);
        
        while(fin.get() != '\n') {
            fin >> neighbor;
            sg[sid].addNeighbor(neighbor);
        }
    }
    
#ifdef VERBOSE
    printf("[ ] %ld switches as below:\n", sg.size());
    for(auto it : sg) {
        int s1 = it.first;
        printf("    - %d -", s1);
        for(auto s2 : sg[s1].getNeighbors()) {
            printf(" %d", s2);
        }
        printf("\n");
    }
#endif
    
    // add rules
    int nr;
    while(ns--) {
        fin >> sid >> nr;
        while(nr--) {
            int rid;
            string prefix;
            int in_port, out_port, priority;
            fin >> rid >> prefix >> in_port >> out_port >> priority;
            // skip edge rules
            if(out_port == PORT_HOST) {
                continue;
            }
            rg[rid] = RuleNode(rid, sid, prefix, in_port, out_port, priority);
            sg[sid].addRule(rid);
        }
    }

#ifdef VERBOSE
    printf("[ ] %ld rules as below:\n", rg.size());
    for(auto it : sg) {
        int s = it.first;
        printf("    - %d -", s);
        for(auto r : sg[s].getRules()) {
            printf(" %d", r);
        }
        printf("\n");
    }
#endif

    // build rule graph
    for(auto it : sg)  {
        int s1 = it.first;
        for(auto s2 : sg.at(s1).getNeighbors()) {
            // for r1 on s1 pointing to s2
            for(auto r1 : sg.at(s1).getRules()) {
                Rule rule1 = rg.at(r1).getRule(); 
                if(rule1.getOutPort() != s2) continue;
                // for r2 on s2 pointed by s1
                for(auto r2 : sg.at(s2).getRules()) {
                    Rule rule2 = rg.at(r2).getRule(); 
                    if(rule2.getInPort() != s1) continue;
                    // build a directed edge if two neighboring rules match

                    if(HSA::matchable(rule1.getOutHeader(), rule2.getInHeader())) {
                        rg[r1].addNext(r2);
                    }
                }
            }
        }
    }

#ifdef VERBOSE
    printf("[ ] rule graph as below:\n");
    for(auto it : rg) {
        int r1 = it.first;
        printf("    - %d -", r1);
        for(auto r2 : rg[r1].getNexts()) {
            printf(" %d", r2);
        }
        printf("\n");
    }
#endif
}

void IO::StoreSwitchHeaders(const string &name, Assignments &a, SwitchTestHeaders &sth)
{
    ofstream fout("../data/store/" + name);
    if(!fout) {
        throw "directory does not exist. IO::StoreSwitchHeaders() exits.";
    }

    int masklen = a[SID_OF_MASKLEN].first;
    fout << masklen << endl;

    for(auto it : sth) {
        int s = it.first;
        fout << s << " " << a[s].first << " " << a[s].second << " " << HSA::stringify(sth[s]) << endl;
    }
    
    printf("[ ] write /data/store/%s\n", name.c_str());
}

void IO::StorePathHeaders(const string &name, PathPacketHeaders &pph, PathTestHeaders &pth)
{
    ofstream fout("../data/store/" + name);
    if(!fout) {
        throw "directory does not exist. IO::StorePathHeaders() exits.";
    }

    for(auto it : pph) {
        auto p = it.first;

        string sp;
        for(auto r : p) {
            fout << sp << r;
            sp = "|";
        }

        fout << " " << HSA::stringify(pph[p]) << " " << HSA::stringify(pth[p]) << endl;
    }
    
    printf("[ ] write /data/store/%s\n", name.c_str());
}

void IO::QuickLoad(const string &name, SwitchGraph &sg, TargetsSet &tss)
{
    ifstream fin("../data/tss/" + name);
    if(!fin) {
        throw "tss file does not exist.";
    }

    // build switch graph
    int ns;
    fin >> ns;
    
    int sid, neighbor;
    for(int idx = 0; idx < ns; idx++) {
        fin >> sid;
        sg[sid] = SwitchNode(sid, idx);
        
        while(fin.get() != '\n') {
            fin >> neighbor;
            sg[sid].addNeighbor(neighbor);
        }
    }
    
    // load targets set (with edge rules excluded and single switch added)
    int np;
    fin >> np;
    
    while(np--) {
        vector<int> targets;
        do {
            fin >> sid;
            targets.push_back(sid);
        } while(fin.get() != '\n');
        
        tss.insert(targets);
    }
}

void IO::StoreTargetsSet(const string &name, SwitchGraph &sg, TargetsSet &tss)
{
    ofstream fout("../data/tss/" + name);
    if(!fout) {
        throw "directory does not exist. IO::StoreTargetsSet() exits.";
    }
    
    // switch graph
    fout << sg.size() << endl;
    for(auto it : sg) {
        int sid = it.first;
        fout << sid;
        for(auto neighbor : sg.at(sid).getNeighbors()) {
            fout << " " << neighbor;
        }
        fout << endl;
    }

    // targets set
    fout << tss.size() << endl;
    for(auto ts : tss) {
        string sp;
        for(auto s : ts) {
            fout << sp << s;
            sp = " ";
        }
        fout << endl;
    }
    
    printf("[ ] write /data/tss/%s\n", name.c_str());
}
