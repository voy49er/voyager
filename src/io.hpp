#ifndef IO_H
#define IO_H

#include <fstream>

#include "structs.hpp"
#include "hsa.hpp"

class IO {
public:
    IO(const IO&)=delete;
    IO& operator=(const IO&)=delete;
    static IO& instance() {
        static IO io;
        return io;
    }
    
    static void LoadTopo(const string &name, SwitchGraph &sg, RuleGraph &rg);
    static void StoreSwitchHeaders(const string &name, Assignments &a, SwitchTestHeaders &sth);
    static void StorePathHeaders(const string &name, PathPacketHeaders &pph, PathTestHeaders &pth);
    
    static void QuickLoad(const string &name, SwitchGraph &sg, TargetsSet &tss);
    static void StoreTargetsSet(const string &name, SwitchGraph &sg, TargetsSet &tss);

private:
    IO(){};
};

#endif
