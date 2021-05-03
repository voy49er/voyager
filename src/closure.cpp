#include "core.hpp"

static void bfs(RuleGraph &rg, int src, TransPath &transpath);
void TransClosure(RuleGraph &rg, TransPath &transpath)
{
    for(auto it : rg) {
        int r = it.first;
        bfs(rg, r, transpath);
    }

#ifdef VERBOSE
    printf("[ ] transitive closure of rule graph as below:\n");
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

void bfs(RuleGraph &rg, int src, TransPath &transpath)
{
    queue<int> q;
    q.push(src);

    HeaderMap in_header;    // reachable header
    HeaderMap out_header;   // set-field(reachable header)
    in_header[src] = rg.at(src).getRule().getInHeader();
    out_header[src] = rg.at(src).getRule().getOutHeader();
    
    set<int> vis;
    while(!q.empty()) {
        int u = q.front();
        q.pop();
        vis.insert(u);

        for(auto v : rg.at(u).getNexts()) {
            in_header[v] = rg.at(v).getRule().getInHeader();
            // out_header[v] won't be used if 'v' is unreachable

            if(HSA::matchable(out_header[u], in_header[v])) {
                // TODO: if v is reachable from both u1 and u2, how to determine whether
                // src->u1->v or src->u2->v? the path with larger header space? the more
                // critical rule of u1 and u2 that deserves more tests? 
                
                // build edge if 'v' is reachable from 'src'
                // 'v' can be in the 'nexts' of 'src' already in two cases:
                // 1. 'v' and 'src' are on the neighboring switches
                // 2. 'v' is reachable from both u1 and u2
                rg.at(src).addNextIfNotExists(v);      
                transpath[src][v] = u;  // 'src' -> ... -> 'u' -> 'v'
                
                // in_header[v] and out_header[v] shouldn't be updated if 'v' is unreachable
                in_header[v] = HSA::intersection(out_header[u], in_header[v]);
                out_header[v] = rg.at(v).getRule().getAvailableOutHeader(in_header[v]);

                // enqueue reachable 'v'
                if(vis.find(v) == vis.end()) {
                    q.push(v);
                    vis.insert(v);
                }
            }
        }
    }
}
