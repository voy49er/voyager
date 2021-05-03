#include "core.hpp"

static set<int> vis;
static unordered_map<int, int> cx, cy;  // match
static HeaderMap in_header;             // reachable header
static HeaderMap out_header;            // set-field (reachable header)

static bool dfs(RuleGraph &rg, int src);
void Hungarian(RuleGraph &rg, unordered_map<int, int> &match)
{
    for(auto it : rg) {
        int src = it.first;
        // aiming at a path cover on DAG, maximum matching is after a transformation where 
        // v is split into vx and vy, edge(v1, v2) is built as edge(v1x, v2y), and thus the
        // DAG becomes a bipartite graph
        cx[src] = cy[src] = -1;
        in_header[src] = rg.at(src).getRule().getInHeader();
        out_header[src] = rg.at(src).getRule().getOutHeader();
    }

    for(auto it : rg) {
        int src = it.first;
        vis.clear();
        dfs(rg, src);   // 'src' must be unmatched
    }

    match = cx;

#ifdef VERBOSE
    printf("[ ] maximum matching as below:\n");
    for(auto it : match) {
        printf("    - %d - %d\n", it.first, it.second);
    }
#endif
}

bool dfs(RuleGraph &rg, int src)
{
    for(auto v : rg.at(src).getNexts()) {
        if(vis.find(v) != vis.end()) continue;
        
        if(HSA::matchable(out_header[src], in_header[v])) {
            vis.insert(v);
            
            // 'v' is unmatched or can be unmatched 
            if(cy[v] == -1 || dfs(rg, cy[v])) {
                // Do NOT shrink in_header[v] recursively
                // instead, reset its in header here before it's (re)matched
                in_header[v] = HSA::intersection(out_header[src], rg.at(v).getRule().getInHeader());
                out_header[v] = rg.at(v).getRule().getAvailableOutHeader(in_header[v]);
                
                // (re)match 'v' to 'src'
                cx[src] = v;
                cy[v] = src;

                return true;
            }
        }
    }

    return false;
}
