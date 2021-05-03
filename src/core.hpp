#ifndef CORE_H
#define CORE_H

#include "structs.hpp"
#include "io.hpp"
#include "hsa.hpp"

/* path cover */
void TopoSort(RuleGraph &rg, vector<int> &topoorder);
void TransClosure(RuleGraph &rg, TransPath &transpath);
void Hungarian(RuleGraph &rg, unordered_map<int, int> &match);
void HopcroftKarp(RuleGraph &rg, unordered_map<int, int> &match);

void PathCover(RuleGraph &rg, PathSet &ps, bool fast);

/* report header assignment */
void BruteForceCompactColoring(SwitchGraphAlpha &alpha, SwitchGraphBeta &beta, CompactColoring &coloring);
void GreedyCompactColoring(SwitchGraphAlpha &alpha, SwitchGraphBeta &beta, CompactColoring &coloring);

void GreedyColoring(SwitchGraphAlpha &alpha, Coloring &coloring);
void Edmonds(SwitchGraphMatrix &beta, int n, vector<int> &vertices, unordered_map<int, int> &match);

void SimpleAssignment(SwitchGraph &sg, Assignments &a);
void GreedyColoringAssignment(SwitchGraph &sg, TargetsSet &tss, Assignments &a);
void CompactColoringAssignment(SwitchGraph &sg, TargetsSet &tss, bool opt, Assignments &a);
void TwoPhaseAssignment(SwitchGraph &sg, TargetsSet &tss, Assignments &a);

void QuickAssignment(SwitchGraph &sg, TargetsSet &tss, string mode, Assignments &a);
void ReportHeaderAssignment(SwitchGraph &sg, RuleGraph &rg, PathSet &ps, string mode, Assignments &a);

/* header calculation */
vector<int> GetTargets(RuleGraph &rg, vector<int> &path);
set<int> GetReporters(SwitchGraph &sg, vector<int> &targets);

void SwitchHeadersCalculation(SwitchGraph &sg, Assignments &a, SwitchTestHeaders &sth);
void PathHeadersCalculation(SwitchGraph &sg, RuleGraph &rg, Assignments &a, PathSet &ps, PathPacketHeaders &pph, PathTestHeaders &pth);

#endif
