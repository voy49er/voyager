#include "core.hpp"

#include "unistd.h"
#include "stdlib.h"
#include <chrono>

#define VSTAT(stat) {if(verbose){stat}}

void usage()
{
    printf("[-] Usage: ./setup -f <topofile> -m <mode> -p <threshold>\n"
           "[ ] <topofile>: filename under /data/topo/\n"
           "[ ] <mode>: simple|greedy|compact\n"
           "[ ] <threshold>: non-negative path length threshold (0 as infinity)\n");
}

int main(int argc, char** argv)
{
    string name;
    string mode;
    unsigned plt = INF;     // path length threshold
    int verbose = 0;

    int opt;
    while((opt = getopt(argc, argv, "f:m:p:v")) != -1) {
       switch(opt) {
           case 'f': name = optarg; break;
           case 'm': mode = optarg; break;
           case 'p': plt = atoi(optarg); break;
           case 'v': verbose = 1; break;
           default: usage(); return 0;
       }
    }

    if(name.empty()) { usage(); return 0; }
    if(mode.empty()) { mode = "compact"; }
    if(plt <= 0) { plt = INF; }
    
    try {
    
    auto st = chrono::high_resolution_clock::now();  // start clock
    
    /* load topo */ 
    VSTAT(printf("[ ] load topology [%s]...\n", name.c_str());)
    SwitchGraph sg;
    RuleGraph rg;
    IO::instance().LoadTopo(name, sg, rg);
    
    /* path cover */
    VSTAT(printf("[ ] solve path cover...\n");)
    PathSet ps;
    PathCover(rg, ps, true);

    /* split paths */
    VSTAT(printf("[ ] split paths...\n");)
    PathSet split_ps;
    for(auto p : ps) {
        for(auto iter = p.begin(); iter < p.end(); iter += plt) {
            split_ps.push_back(vector<int>(iter, iter + plt >= p.end() ? p.end() : iter + plt));
        }
    }

    /* assign report headers */
    VSTAT(printf("[ ] assign report headers...\n");)
    Assignments a;
    ReportHeaderAssignment(sg, rg, split_ps, mode, a);
    
    VSTAT(printf("\033[32m[!] %d monitoring bits required.\033[0m\n", a[SID_OF_MASKLEN].first);)
    
    /* calculate headers */
    VSTAT(printf("[ ] calculate headers...\n");)
    SwitchTestHeaders sth;
    SwitchHeadersCalculation(sg, a, sth);
    
    PathPacketHeaders pph;
    PathTestHeaders pth;
    PathHeadersCalculation(sg, rg, a, split_ps, pph, pth);

    /* persist headers */
    name.replace(name.end()-5, name.end(), "");     // remove ".topo"
    IO::instance().StoreSwitchHeaders(name + ".switch.store", a, sth);
    IO::instance().StorePathHeaders(name + ".path.store", pph, pth);

    chrono::duration<double, milli> drt = chrono::high_resolution_clock::now() - st;
    
    printf("\033[32m[!] [%s] [%s] [p=%d] [b=%d] [t=%f]\033[0m\n", (name+".topo").c_str(), mode.c_str(), plt, a[SID_OF_MASKLEN].first, float(drt.count()) / 1000.0); 
    }
    catch(const char* err) {
        printf("\033[31m[x] error: %s\033[0m\n", err);
    }
    
    return 0;
}
