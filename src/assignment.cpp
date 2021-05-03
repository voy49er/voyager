#include "core.hpp"

static void BuildAlpha(SwitchGraph &sg, SwitchGraphAlpha &alpha, TargetsSet &tss);
static void BuildBeta(SwitchGraph &sg, SwitchGraphBeta &beta, TargetsSet &tss);
static void BuildMatrix(SwitchGraph &sg, SwitchGraphMatrix &matrix, TargetsSet &tss, unordered_map<int, int> &s_root);

void QuickAssignment(SwitchGraph &sg, TargetsSet &tss, string mode, Assignments &a)
{
    if(mode == "greedy") {
        GreedyColoringAssignment(sg, tss, a);
    }
    else if(mode == "compact") {
        CompactColoringAssignment(sg, tss, false, a);
    }

#ifdef DEBUG
    printf("[ ] targets and reporters set as below:\n");
    for(auto ts : tss) {
        printf("    -");
        for(auto s : ts) {
            printf(" %d", s);
        }
        printf(" -");
        for(auto s : GetReporters(sg, ts)) {
            printf(" %d", s);
        }
        printf("\n");
    }
#endif
}

void ReportHeaderAssignment(SwitchGraph &sg, RuleGraph &rg, PathSet &ps, string mode, Assignments &a)
{
    TargetsSet tss;
    for(auto p : ps) {
        // targets derived from tested path
        vector<int> targets = GetTargets(rg, p);
        tss.insert(targets);
    }
    
    for(auto it : sg) {
        int s = it.first;
        // single switch as targets
        vector<int> targets = {s};
        tss.insert(targets);
    }

#ifdef DEBUG
    printf("[ ] targets and reporters set as below:\n");
    for(auto ts : tss) {
        printf("    -");
        for(auto s : ts) {
            printf(" %d", s);
        }
        printf(" -");
        for(auto s : GetReporters(sg, ts)) {
            printf(" %d", s);
        }
        printf("\n");
    }
#endif
    
    if(mode == "simple") {
        SimpleAssignment(sg, a);
    }
    else if(mode == "greedy") {
        GreedyColoringAssignment(sg, tss, a);
    }
    else if(mode == "compact") {
        CompactColoringAssignment(sg, tss, false, a);
    }
    else if(mode == "compact-opt") {
        CompactColoringAssignment(sg, tss, true, a);
    }
    else if(mode == "two-phase") {
        TwoPhaseAssignment(sg, tss, a);
    }
    else {
        throw "undefined mode. ReportHeaderAssignment() exits.";
    }

#ifdef DEBUG
    printf("[!] %d bits required for report header\n", a[SID_OF_MASKLEN].first);
    printf("    switch report headers as below:\n");
    for(auto it : a) {
        int s = it.first;
        if(s != SID_OF_MASKLEN) {
            printf("    - %d - (%d, %d)\n", s, a[s].first, a[s].second);
        }
    }
#endif
}

/* simple assignment: a unique maskbit for each switch */
void SimpleAssignment(SwitchGraph &sg, Assignments &a)
{
    int maskbit = 0;
    for(auto it : sg) {
        int s = it.first;
        a[s] = make_pair(maskbit, 1);
        maskbit++;
    }

    a[SID_OF_MASKLEN] = make_pair(maskbit, 0);
}

/* conventional greedy coloring on alpha without the help of beta */
void GreedyColoringAssignment(SwitchGraph &sg, TargetsSet &tss, Assignments &a)
{
    // step 1: build alpha
    SwitchGraphAlpha alpha;
    BuildAlpha(sg, alpha, tss);
    
    // step 2: coloring
    Coloring coloring;
    GreedyColoring(alpha, coloring);

    int cmax = -1;
    for(auto it : coloring) {
        int color = it.first;
        cmax = (cmax < color) ? color : cmax;
        for(auto s : coloring.at(color)) {
            a[s] = make_pair(color, 1);
        }
    }

    a[SID_OF_MASKLEN] = make_pair(cmax + 1, 0);
}

/* 
 * compact coloring assignment: coloring and matching at the same time
 * - brute force: find the optimum in exponential time
 * - greedy: try to approximate the optimum in polynomial time
 *
 */
void CompactColoringAssignment(SwitchGraph &sg, TargetsSet &tss, bool opt, Assignments &a)
{
    SwitchGraphAlpha alpha;
    BuildAlpha(sg, alpha, tss);

    SwitchGraphBeta beta;
    BuildBeta(sg, beta, tss);
    
    CompactColoring coloring;
    if(opt) {
        BruteForceCompactColoring(alpha, beta, coloring);
    }
    else {
        GreedyCompactColoring(alpha, beta, coloring);
    }
    int cmax = -1;
    for(auto it : coloring) {
        Color color = it.first;
        cmax = (cmax < color.first) ? color.first : cmax;
        for(auto s : coloring.at(color)) {
            a[s] = color;
        }
    }

    a[SID_OF_MASKLEN] = make_pair(cmax + 1, 0);

#ifdef DEBUG
    printf("[ ] compact coloring as below:\n");
    for(auto it : coloring) {
        printf("    -");
        for(auto s : coloring.at(it.first)) {
            printf(" %d", s);
        }
        printf(" - (%d, %d)\n", it.first.first, it.first.second);
    }
#endif
}

/* 
 * 2-phase assignment: greedy coloring + maximum matching
 * 
 * phase 1: Merge identical vertices.
 * - step 1: build alpha. Incident vertices can't hold the same value and mask.
 * - step 2: coloring. Greedy coloring doesn't guarantee the optimum.
 * - step 3: merge. Vertices are considered as identical if with the same color.
 * 
 * phase 2: Match cooperative vertices.
 * - step 1: build matrix. Incident vertices can share a maskbit with different value.
 * - step 2: maximum matching. Vertices are considered as cooperative if matched.
 *
 */
void TwoPhaseAssignment(SwitchGraph &sg, TargetsSet &tss, Assignments &a)
{
    int n = sg.size();

    /* Merge phase */ 

    // step 1: build alpha
    SwitchGraphAlpha alpha;
    BuildAlpha(sg, alpha, tss);

    // step 2: coloring
    Coloring coloring;
    GreedyColoring(alpha, coloring);

    // step 3: merge
    unordered_map<int, int> s_root;
    vector<int> s_remained;
    for(auto it : coloring) {
        int c = it.first;
        // the first one as a representative
        int root = coloring.at(c)[0];
        for(auto s : coloring.at(c)) {
            s_root[s] = root; 
        }

        s_remained.push_back(root);
    }
   
#ifdef DEBUG
    printf("[ ] merge phase (coloring) as below:\n");
    for(auto it : coloring) {
        printf("    -");
        for(auto s : coloring.at(it.first)) {
            printf(" %d", s);
        }
        printf(" - %d\n", it.first);
    }
#endif 

    /* Match phase */

    // step 1: build matrix as beta's complement 
    SwitchGraphMatrix matrix;
    BuildMatrix(sg, matrix, tss, s_root);

    // step 2: maximum matching
    unordered_map<int, int> match;
    vector<int> remained;
    for(auto s : s_remained) {
        remained.push_back(sg.at(s).getSIdx());
    }
    Edmonds(matrix, n, remained, match);

#ifdef DEBUG
    printf("[ ] match phase (maximum matching) as below:\n");
    if(match.size() == 0) {
        printf("    - match is empty\n");
    }
    else {
        printf("    -");
        for(auto s : s_remained) {
            printf(" %d#%d", s, sg.at(s).getSIdx());
        }
        printf("\n");
        for(auto it : match) {
            printf("    - #%d - #%d\n", it.first, it.second);
        }
    }
#endif

    /*
     * Final assignments:
     * - matched: a maskbit with value 1 and 0 respectively.
     * - unmatched: a maksbit with value 1.
     * - merged: the same maskbit and value as its representative.
     *
     */
    int maskbit = 0;
    Assignments aidx;       // assign to 'remained' idx first

    set<int> matched;       // a subset of 'remained'
    for(auto it : match) {
        int idx = it.first;
        // for matched idx
        aidx[idx] = make_pair(maskbit, 1);
        aidx[match[idx]] = make_pair(maskbit, 0);
        maskbit++;

        matched.insert(idx);
        matched.insert(match[idx]);
    }

    for(auto idx : remained) {
        // for unmatched idx
        if(matched.find(idx) == matched.end()) {
            aidx[idx] = make_pair(maskbit, 1);
            maskbit++;
        }
    }

    // for sid (merged vertices is handled here) 
    for(auto it : sg) {
        int s = it.first;
        a[s] = aidx[sg.at(s_root[s]).getSIdx()];
    }

    a[SID_OF_MASKLEN] = make_pair(maskbit, 0);
}

void BuildAlpha(SwitchGraph &sg, SwitchGraphAlpha &alpha, TargetsSet &tss)
{
    for(auto targets : tss) {
        set<int> reporters = GetReporters(sg, targets);
        
        // two vertices can't be identical iff
        // one is the target and the other is its reporter 
        for(auto s1 : targets) {
            for(auto s2 : reporters) {
                alpha[s1].insert(s2);
                alpha[s2].insert(s1);
            }
        }
    }

#ifdef DEBUG
    printf("[ ] graph alpha as below:\n");
    for(auto it : alpha) {
        int s1 = it.first;
        printf("    - %d -", s1);
        for(auto s2 : alpha[s1]) {
            printf(" %d", s2);
        }
        printf("\n");
    }
#endif
}

void BuildBeta(SwitchGraph &sg, SwitchGraphBeta &beta, TargetsSet &tss)
{
    for(auto targets : tss) {
        // two vertices can't be cooperative iff in two cases
        
        // case 1: they are in the same target set
        for(auto s1 : targets) {
            for(auto s2 : targets) {
                beta[s1].insert(s2);
                beta[s2].insert(s1);
            }
        }

        // case 2: they are in the same reporter set 
        set<int> reporters = GetReporters(sg, targets);
        for(auto s1 : reporters) {
            for(auto s2 : reporters) {
                beta[s1].insert(s2);
                beta[s2].insert(s1);
            }
        }
    }


#ifdef DEBUG
    printf("[ ] graph beta as below:\n");
    if(beta.size() == 0) {
        printf("    - beta is empty\n");
    }
    for(auto it : beta) {
        int s1 = it.first;
        printf("    - %d -", s1);
        for(auto s2 : beta[s1]) {
            printf(" %d", s2);
        }
        printf("\n");
    }
#endif
}

void BuildMatrix(SwitchGraph &sg, SwitchGraphMatrix &matrix, TargetsSet &tss, unordered_map<int, int> &s_root)
{
    for(auto it1 : sg) {
        for(auto it2 : sg) {
            int s1 = it1.first;
            int s2 = it2.first;
            int idx1 = sg.at(s_root[s1]).getSIdx();
            int idx2 = sg.at(s_root[s2]).getSIdx();
            if(idx1 != idx2) {
                matrix[idx1][idx2] = matrix[idx2][idx1] = 1;
            }
        }
    }

    for(auto targets : tss) {
        // two vertices can't be cooperative iff in two cases
        
        // case 1: they are in the same target set
        for(auto s1 : targets) {
            for(auto s2 : targets) {
                int idx1 = sg.at(s_root[s1]).getSIdx();
                int idx2 = sg.at(s_root[s2]).getSIdx();
                matrix[idx1][idx2] = matrix[idx2][idx1] = 0;
            }
        }

        // case 2: they are in the same reporter set 
        set<int> reporters = GetReporters(sg, targets);
        for(auto s1 : reporters) {
            for(auto s2 : reporters) {
                int idx1 = sg.at(s_root[s1]).getSIdx();
                int idx2 = sg.at(s_root[s2]).getSIdx();
                matrix[idx1][idx2] = matrix[idx2][idx1] = 0;
            }
        }
    }

#ifdef DEBUG
    printf("[ ] graph matrix as below:\n");
    int n = sg.size();
    printf("    -");
    for(auto it : sg) {
        int s = it.first;
        printf(" %d#%d", s, sg.at(s).getSIdx());
    }
    printf("\n");
    for(int idx1 = 0; idx1 < n; idx1++) {
        printf("    -");
        for(int idx2 = 0; idx2 < n; idx2++) {
            printf(" %d", matrix[idx1][idx2]);
        }
        printf("\n");
    }
#endif
}
