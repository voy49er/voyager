#include "core.hpp"

dynbitset GetPacketHeader(RuleGraph &rg, vector<int> path);
dynbitset GetTestHeader(SwitchGraph &sg, Assignments &a, vector<int> targets);

void SwitchHeadersCalculation(SwitchGraph &sg, Assignments &a, SwitchTestHeaders &sth)
{
    for(auto it : sg) {
        int s = it.first;
        vector<int> targets = {s};
        sth[s] = GetTestHeader(sg, a, targets);
    }
    
#ifdef DEBUG
    printf("[ ] switch test headers calculated as below:\n");
    for(auto it : a) {
        int s = it.first;
        if(s != SID_OF_MASKLEN) {
            printf("    - %d - %s\n", s, HSA::stringify(sth[s]).c_str());
        }
    }
#endif
}

void PathHeadersCalculation(SwitchGraph &sg, RuleGraph &rg, Assignments &a, PathSet &ps, PathPacketHeaders &pph, PathTestHeaders &pth)
{
    for(auto p : ps) {
        pph[p] = GetPacketHeader(rg, p);

        vector<int> targets = GetTargets(rg, p);
        pth[p] = GetTestHeader(sg, a, targets);
    }

#ifdef DEBUG
    printf("[ ] path packet headers and test headers calculated as below:\n");
    for(auto p : ps) {
        printf("    - <");
        for(auto x : p) {
            printf(" %d(%d)", x, rg.at(x).getRule().getSID());
        }
        printf(" > - %s - %s\n", HSA::stringify(pph[p]).c_str(), HSA::stringify(pth[p]).c_str());
    }
#endif
}

dynbitset GetPacketHeader(RuleGraph &rg, vector<int> path)
{
    dynbitset ph;

    // TODO: handle set-field
    // observation: after a rule that sets a bit, the later reachability is independent
    // of the original bit value, and ensured when path cover is solved
    // solution: bit-by-bit initialization. h[i] = intersection(r1[i], ..., rk[i])
    // where rk is the first rule that sets ith bit.
    for(auto r : path) {
        dynbitset rh = rg.at(r).getRule().getInHeader();
        ph = (r == path[0]) ? rh : HSA::intersection(ph, rh);
    }

    return ph;
}

dynbitset GetTestHeader(SwitchGraph &sg, Assignments &a, vector<int> targets)
{
    int masklen = a[SID_OF_MASKLEN].first;
    dynbitset th(masklen * 2);
    
    for(size_t ith = 0; ith < th.size() / 2; ith++) {
        HSA::set(th, ith, 'x');
    }
    
    // violate all targets to silence them
    for(auto s : targets) {
        if(!HSA::set(th, a[s].first, a[s].second ? '0' : '1')) {
            throw "GetTestHeader() failed on targets.";

#ifdef DEBUG
    printf("[!] fail to violate target %d, a[s]=(%d,%d), hdr=%s\n", s, a[s].first, a[s].second, HSA::stringify(th).c_str());
    printf("[ ] targets -");
    for(auto t : targets) {
        printf(" %d(%d,%d)", t, a[t].first, a[t].second);
    }
    printf("\n[ ] reporters -");
    for(auto r : GetReporters(sg, targets)) {
        printf(" %d(%d,%d)", r, a[r].first, a[r].second);
    }
    printf("\n");
#endif
        }
    }
    
    // obey all reporters to activate them
    set<int> reporters = GetReporters(sg, targets);
    for(auto s : reporters) {
        if(!HSA::set(th, a[s].first, a[s].second ? '1' : '0')) {
            throw "GetTestHeader() failed on reporters.";

#ifdef DEBUG 
    printf("[!] fail to obey reporter %d, a[s]=(%d,%d), hdr=%s\n", s, a[s].first, a[s].second, HSA::stringify(th).c_str());
    printf("[ ] targets -");
    for(auto t : targets) {
        printf(" %d(%d,%d)", t, a[t].first, a[t].second);
    }
    printf("\n[ ] reporters -");
    for(auto r : reporters) {
        printf(" %d(%d,%d)", r, a[r].first, a[r].second);
    }
    printf("\n");
#endif
        }
    }

    return th;
}

vector<int> GetTargets(RuleGraph &rg, vector<int> &path)
{
    vector<int> targets;
    for(auto r : path) {
        targets.push_back(rg.at(r).getRule().getSID());
    }

    return targets;
}

set<int> GetReporters(SwitchGraph &sg, vector<int> &targets)
{
    set<int> reporters;
    
    for(auto s : targets) {
        for(auto n : sg.at(s).getNeighbors()) {
            reporters.insert(n);
        }
    }
    
    for(auto s : targets)  {
        reporters.erase(s);
    }

    return reporters;
}
