#include "core.hpp"

void TopoSort(RuleGraph &rg, vector<int> &topoorder)
{
    // get indegree
    unordered_map<int, int> indegree;
    
    for(auto it : rg) {
        int r = it.first;
        indegree[r] = 0;
    }

    for(auto it : rg) {
        int u = it.first;
        for(auto v : rg.at(u).getNexts()) {
            indegree[v]++;
        }
    }

    // start
    queue<int> q;

    for(auto it : rg) {
        int r = it.first;
        if(indegree[r] == 0) {
            q.push(r);
        }
    }

    while(!q.empty()) {
        int u = q.front();
        q.pop();

        topoorder.push_back(u);

        for(auto v : rg.at(u).getNexts()) {
            indegree[v]--;
            if(indegree[v] == 0) {
                q.push(v);
            }
        }
    }
}
