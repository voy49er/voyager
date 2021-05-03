#include "core.hpp"

void usage()
{
    printf("[-] Usage: ./tss -f <topofile>|<tssfile> -m <mode>\n"
           "[ ] <topofile>: filename under /data/topo/ (with mode store)\n"
           "[ ] <tssfile>: filename under /data/tss/ (with mode greedy|compact|compare)\n"
           "[ ] <mode>: store|greedy|compact|compare\n");
}

void store(string name)
{
    /* load topo */ 
    SwitchGraph sg;
    RuleGraph rg;
    IO::instance().LoadTopo(name, sg, rg);

    /* path cover (hopcroftkarp) */
    PathSet ps;
    PathCover(rg, ps, true);
    
    /* get and persist targets set */
    TargetsSet tss;
    for(auto p : ps) {
        vector<int> targets = GetTargets(rg, p);
        tss.insert(targets);
    }

    for(auto it : sg) {
        int s = it.first;
        vector<int> targets = {s};
        tss.insert(targets);
    }

    name.replace(name.end()-5, name.end(), "");
    IO::instance().StoreTargetsSet(name + ".tss", sg, tss);
}

void assign(string name, string mode)
{
    /* load topo and targets set */
    SwitchGraph sg;
    TargetsSet tss;
    IO::instance().QuickLoad(name, sg, tss);
    
    /* do assignment */
    Assignments a;
    QuickAssignment(sg, tss, mode, a); 
    
    printf("\033[32m[!] [%s] [%s] [b=%d]\033[0m\n", name.c_str(), mode.c_str(), a[SID_OF_MASKLEN].first);
}

void compare(string name)
{
    /* load topo and targets set */
    SwitchGraph sg;
    TargetsSet tss;
    IO::instance().QuickLoad(name, sg, tss);
    
    /* do assignment (greedy, hopcroftkarp) */
    Assignments ag;
    QuickAssignment(sg, tss, "greedy", ag); 
    
    Assignments ac;
    QuickAssignment(sg, tss, "compact", ac);
    
    printf("\033[32m[!] [%s] [greedy=%d] [compact=%d]\033[0m\n", name.c_str(), ag[SID_OF_MASKLEN].first, ac[SID_OF_MASKLEN].first); 
}

int main(int argc, char** argv)
{
    string name;
    string mode;
    
    int opt;
    while((opt = getopt(argc, argv, "f:m:")) != -1) {
        switch(opt) {
            case 'f': name = optarg; break;
            case 'm': mode = optarg; break;
            default: usage(); return 0;
        }
    }
    
    try {
        if(mode == "store") {
            store(name);
        }
        else if(mode == "compare") {
            compare(name);
        }
        else if(mode == "greedy" || mode == "compact") {
            assign(name, mode);
        }
        else {
            usage();
            return 0;
        }

    }
    catch(const char* err) {
        printf("\033[31m[x] error: %s\033[0m\n", err);
    }

    return 0;
}
