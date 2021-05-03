#include "core.hpp"

static int dist;
static unordered_map<int, int> d;                  // distance

static set<int> vis;
static unordered_map<int, int> cx, cy;             // match

static HeaderMap in_header;    // reachable header
static HeaderMap out_header;   // set-field (reachable header)

static bool bfs(RuleGraph &rg);
static bool dfs(RuleGraph &rg, int src);
void HopcroftKarp(RuleGraph &rg, unordered_map<int, int> &match)
{
    for(auto it : rg) {
        int src = it.first;
        cx[src] = cy[src] = -1;     // vertex split
        in_header[src] = rg.at(src).getRule().getInHeader();
        out_header[src] = rg.at(src).getRule().getOutHeader();
    }
    
    while(bfs(rg)) {
        for(auto it : rg) {
            int src = it.first;
            if(cx[src] == -1) {
                vis.clear();
                dfs(rg, src);
            }
        }
    }

    match = cx;

#ifdef VERBOSE
    printf("[ ] maximum matching as below:\n");
    for(auto it : match) {
        printf("    - %d - %d\n", it.first, it.second);
    }
#endif
}

bool bfs(RuleGraph &rg)
{
   queue<int>  q;
   for(auto it : rg) {
       int u = it.first;
       if(cx[u] == -1) {
           d[u] = 0;
           q.push(u);
       }
       else {
           d[u] = INF; 
       }
   }

   dist = INF;
    
   while(!q.empty()) {
       int u = q.front();
       q.pop();
        
       // 'd[v]' must be 'dist' + 1 if 'd[u]' == 'dist'
       if(d[u] >= dist) break;

       for(auto v : rg.at(u).getNexts()) {
           if(HSA::matchable(out_header[u], in_header[v])) {
               if(cy[v] == -1 && dist == INF) {
                   dist = d[u] + 1;
               }
               else if(cy[v] != -1 && d[cy[v]] == INF){
                   d[cy[v]] = d[u] + 1;
                   q.push(cy[v]);
               }
           }
       }
   }

   return dist != INF;
}

bool dfs(RuleGraph &rg, int src)
{
    for(auto v : rg.at(src).getNexts()) {
        if(vis.find(v) != vis.end()) continue;
        
        if(HSA::matchable(out_header[src], in_header[v])) {
            vis.insert(v);
            
            // search only the 'dist'th layer
            if(cy[v] == -1 && dist != d[src] + 1) continue;
            if(cy[v] != -1 && d[cy[v]] != d[src] + 1) continue;

            // prune 'v' if it is matched at depth 'dist'
            if(cy[v] != -1 && d[cy[v]] == dist) continue;
            
            if(cy[v] == -1 || dfs(rg, cy[v])) {
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
