#include "core.hpp"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/max_cardinality_matching.hpp>

// <OutEdgeList, VertextList, Directed>
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS> BoostGraph;

void Edmonds(SwitchGraphMatrix &beta, int n, vector<int> &vertices, unordered_map<int, int> &match)
{
    // boost graph representation
    BoostGraph bg(n);
    for(auto u : vertices) {
        for(auto v : vertices) {
            if(beta[u][v]) {
                boost::add_edge(u, v, bg);
            }
        }
    }
    
    // maximum matching
    vector<boost::graph_traits<BoostGraph>::vertex_descriptor> mate(n);
    boost::edmonds_maximum_cardinality_matching(bg, &mate[0]);
    
    // fetch matching 
    boost::graph_traits<BoostGraph>::vertex_iterator vi, vi_end;
    for(boost::tie(vi, vi_end) = boost::vertices(bg); vi != vi_end; vi++) {
        if(mate[*vi] != boost::graph_traits<BoostGraph>::null_vertex()) {
            // record only once
            if(*vi < mate[*vi]) {
                match[*vi] = mate[*vi];
            }
        }
    }
}
