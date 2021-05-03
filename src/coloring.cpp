#include "core.hpp"

static int cmax;
static vector<int> seq;
static unordered_map<int, Color> opt;

static void CountDegrees(SwitchGraphAlpha &alpha, set<pair<int, int>> &degrees);

static void dfs(SwitchGraphAlpha &alpha, SwitchGraphBeta &beta, unordered_map<int, Color> &color, int ith, int c);
void BruteForceCompactColoring(SwitchGraphAlpha &alpha, SwitchGraphBeta &beta, CompactColoring &coloring)
{
    // all vertices are uncolored at first
    unordered_map<int, Color> color; 
    for(auto it : alpha) {
        int s = it.first;
        color[s] = make_pair(-1, 0);
    }
    
    // count degrees
    set<pair<int, int>> degrees;
    CountDegrees(alpha, degrees);
    
    // the chromatic number is at most max(degree) + 1
    // the minimum number of required colors is maintained as cmax + 1
    cmax = degrees.rbegin()->first;
    
    for(auto iter = degrees.rbegin(); iter != degrees.rend(); iter++) {
        int v = iter->second;
        seq.push_back(v);
    }  
    
    dfs(alpha, beta, color, 0, 0);
    
    for(auto it : opt) {
        int v = it.first;
        coloring[opt[v]].push_back(v);
    }
}

int phi(int c, unordered_map<int, Color> &color, int ith)
{
    for(int i = 0; i < ith; i++) {
        if(color[seq[i]].first == c) {
            return i;
        }
    }

    return -1;
}

void dfs(SwitchGraphAlpha &alpha, SwitchGraphBeta &beta, unordered_map<int, Color> &color, int ith, int c)
{
#ifdef DEBUG
    printf("ith=%d, c=%d color={", ith, c);
    for(auto it : color) {
        printf(" %d(%d,%d)", it.first, it.second.first, it.second.second);
    }
    printf(" }\n");
#endif

    if(c > cmax) {
        return;
    }  

    if((unsigned)ith == seq.size()) {
        if(opt.size() == 0 || cmax > c)  {
            cmax = c;
            opt = color;
        }
        
        return;
    }

    // prune using symmetry
    if(ith >= 1) {
        int latest_c = color[seq[ith-1]].first;
        for(int bigger_c = latest_c + 1; bigger_c <= c; bigger_c++) {
            int pos = phi(bigger_c, color, ith-1);
            if(pos != -1 && pos < phi(latest_c, color, ith)) {
                return;
            }
        }
    }

    int v = seq[ith];
    
    set<int> assigned;
    set<int> shareable;
    
    for(auto u : alpha.at(v)) {
        if(color[u].first == -1) continue;
        
        if(color[u].second == 1) {
            assigned.insert(color[u].first);
            
            if(beta[v].find(u) == beta[v].end()) {
                shareable.insert(color[u].first);
            }
        }
    }

    // try all shareable colors (shared with a neighbor and thus in [0, c])
    for(auto sc : shareable) {
        color[v] = make_pair(sc, 0);
        dfs(alpha, beta, color, ith + 1, c);
        color[v] = make_pair(-1, 0);
    }
    
    // try all available colors (different from all neighbors and in [0, c])
    for(int ac = 0; ac <= c; ac++) {
        if(assigned.find(ac) == assigned.end()) {
            color[v] = make_pair(ac, 1);
            dfs(alpha, beta, color, ith + 1, c); 
            color[v] = make_pair(-1, 0); 
        }
    }
    
    // introduce new color c+1
    color[v] = make_pair(c+1, 1);
    dfs(alpha, beta, color, ith + 1, c+1); 
    color[v] = make_pair(-1, 0); 
}

void GreedyCompactColoring(SwitchGraphAlpha &alpha, SwitchGraphBeta &beta, CompactColoring &coloring)
{
    // all vertices are uncolored at first
    unordered_map<int, Color> color; 
    for(auto it : alpha) {
        int s = it.first;
        color[s] = make_pair(-1, 0);
    }

    // count degrees
    set<pair<int, int>> degrees;
    CountDegrees(alpha, degrees);
    
    // coloring from the highest degree vertex
    for(auto iter = degrees.rbegin(); iter != degrees.rend(); iter++) {
        int v = iter->second;
        
        set<int> assigned;
        set<int> possibly_shareable; // subset of 'assigned'
        set<int> not_shareable;
        
        for(auto u : alpha.at(v)) {
            int maskbit = color[u].first;
            int value = color[u].second;

            if(maskbit == -1) continue;
            
            // examine the color assigned to a neighbor in alpha
            if(value == 1) {
                assigned.insert(maskbit);

                // seize the opportunity that 'v' can share the color with value 0
                if(beta[v].find(u) == beta[v].end()) {
                    possibly_shareable.insert(maskbit);
                }
            }
            else {
                // exclude 0-color from 'shareable'
                not_shareable.insert(maskbit);
            }
        }
        
        for(auto u : beta.at(v)) {
            int maskbit = color[u].first;
            int value = color[u].second;

            if(maskbit == -1) continue;
            
            // examine the color assigned to a neighbor in beta
            if(value == 0) {
                // exclude 0-color from NOT 'assigned'
                assigned.insert(maskbit);
            }
            else {
                // exclude 1-color from 'shareable'
                not_shareable.insert(maskbit);
            }
        }
        
        set<int> shareable = possibly_shareable; // subset of 'possibly_shareable', thus 'assigned'
        for(auto nc : not_shareable) {
            shareable.erase(nc);
        }

        int sc = -1;    // lowest shareable color in 'assigned'
        if(shareable.size() > 0) {
            sc = *shareable.begin();
        }

        int ac = 0;     // lowest available color not in 'assigned'
        for(auto c : assigned) {
            if(ac != c) {
                break;
            }
            ac++;
        }
        
        // color 'v' with the lower one of 'sc' and 'ac'
        if(sc != -1 && sc < ac) {
            color[v] = make_pair(sc, 0);    // shareable case, value 0 only
        }
        else {
            color[v] = make_pair(ac, 1);    // available case, value 1 only 
        }
        
        coloring[color[v]].push_back(v);

#ifdef DEBUG
    printf("[ ] color[%d]=(%d,%d) with ac=%d, sc=%d, shareable=[", v, color[v].first, color[v].second, ac, sc);
    for(auto c : shareable) {
        printf(" %d", c);
    }
    printf(" ]\n");
#endif
    }
}

void GreedyColoring(SwitchGraphAlpha &alpha, Coloring &coloring)
{
    // all vertices are uncolored at first
    unordered_map<int, int> color;
    for(auto it : alpha) {
        int s = it.first;
        color[s] = -1;
    }

    // count degrees
    set<pair<int, int>> degrees;
    CountDegrees(alpha, degrees); 

    // coloring from the highest degree vertex
    for(auto iter = degrees.rbegin(); iter != degrees.rend(); iter++) {
        int v = iter->second;

        set<int> assigned;
        for(auto u : alpha.at(v)) {
            if(color[u] != -1) {
                assigned.insert(color[u]);
            }
        }

        int ac = 0;
        for(auto c : assigned) {
            if(ac != c) {
                break;
            }
            ac++;
        }

        // color 'v' with the first available color 'ac'
        color[v] = ac;
        coloring[ac].push_back(v);
    }
}

void CountDegrees(SwitchGraphAlpha &alpha, set<pair<int, int>> &degrees)
{
    for(auto it : alpha) {
        int s = it.first;
        int degree = alpha.at(s).size();
        degrees.insert(make_pair(degree, s));
    }
}
