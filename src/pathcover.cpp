#include "core.hpp"

void PathCover(RuleGraph &rg, PathSet &ps, bool fast)
{
    // step 1: topological sorting
    vector<int> topoorder;
    TopoSort(rg, topoorder);

    if(topoorder.size() < rg.size()) {
        throw "cycle detected. PathCover() exits.";
    }
    
    // step 2: non-disjoint path covering
    // - transitive closure
    TransPath transpath;
    TransClosure(rg, transpath);
    
    // - disjoint path covering on DAG (solved by maximum matching)
    unordered_map<int, int> match;
    fast ? HopcroftKarp(rg, match) : Hungarian(rg, match);

    // - path reconstruction (from 'match' and 'transpath')
    set<int> vis;
    for(auto src : topoorder) {
        if(vis.find(src) != vis.end()) continue;
        vis.insert(src);

        vector<int> path;
        path.push_back(src);
        
        int v = match[src];
        while(v != -1) {
            vis.insert(v);
            
            // expand the transitive path
            vector<int> subpath;
            int end = v;
            while(end != src) {
                subpath.push_back(end);
                end = transpath[src][end];  // trace back in 'transpath'
            }
            path.insert(path.end(), subpath.rbegin(), subpath.rend());
            
            // trace down in 'match'
            src = v;
            v = match[src];
        }

        ps.push_back(path);
    }

#ifdef VERBOSE
    printf("[ ] %ld paths as below:\n", ps.size());
    for(auto p : ps) {
        printf("    - path <");
        for(auto x : p) {
            printf(" %d", x);
        }
    }
#endif 
}
