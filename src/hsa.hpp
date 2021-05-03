#ifndef HSA_H
#define HSA_H

#include "structs.hpp"

class HSA {
public:
    static char get(const dynbitset &a, int ith);   
    static bool set(dynbitset &a, int ith, char ch);
    
    static dynbitset intersection(const dynbitset &a, const dynbitset &b);
    static bool isEmpty(const dynbitset &a);
    static bool matchable(const dynbitset &a, const dynbitset &b);
    
    static dynbitset translate(const string &prefix);
    static string stringify(const dynbitset &header);
};

#endif

