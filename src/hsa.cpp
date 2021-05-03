#include "hsa.hpp"
#include "structs.hpp"

char HSA::get(const dynbitset &a, int ith)
{
    bool hi = a[2*ith + 1];
    bool lo = a[2*ith];

    if(hi == 0 and lo == 0) return '-';
    if(hi == 0 and lo == 1) return '0';
    if(hi == 1 and lo == 0) return '1';
    if(hi == 1 and lo == 1) return 'x';

    throw "undefined character. HSA::get() exits.";
}

    
bool HSA::set(dynbitset &a, int ith, char ch)
{
    if(ch == '0' && get(a, ith) == '1') return false;
    if(ch == '1' && get(a, ith) == '0') return false;
    
    bool hi, lo;
    if(ch == '-') { hi = 0; lo = 0; }
    else if(ch == '0') { hi = 0; lo = 1; }
    else if(ch == '1') { hi = 1; lo = 0; }
    else if(ch == 'x') { hi = 1; lo = 1; }
    else {
        throw "undefined character. HSA::set() exits.";
    }

    a[2*ith + 1] = hi;
    a[2*ith] = lo;

    return true;
}
    
dynbitset HSA::intersection(const dynbitset &a, const dynbitset &b)
{
    return a & b;
}

bool HSA::isEmpty(const dynbitset &a)
{
    for(size_t ith = 0; ith < a.size() / 2; ith++) {
        if(get(a, ith) == '-') {
            return true;
        }
    }

    return false;
}

bool HSA::matchable(const dynbitset &a, const dynbitset &b)
{
    return !isEmpty(intersection(a, b));
}
  
dynbitset HSA::translate(const string &prefix)
{
    string ip;
    int mask_l;
    size_t pos = prefix.find('/');
    
    if(pos == string::npos) {
        ip = prefix;
        mask_l = 32;
    }
    else {
        ip = prefix.substr(0, pos);
        mask_l = stoi(prefix.substr(pos + 1));
    }

    // convert ip to binary string
    string binstr;

    ip += '.';
    do {
        pos = ip.find('.');
        int part = stoi(ip.substr(0, pos));
        
        // from MSB to LSB
        for(int ct = 0; ct < 8; ct++) {
            binstr += (part >> (7-ct) & 1) ? '1' : '0'; 
        }
        
        ip = ip.substr(pos + 1);
    } while(ip != "");

    // set header based on the binary (little endian)
    dynbitset header(64);
    for(int ith = 0; ith < 32; ith++) {
        char ch = (ith < (32-mask_l)) ? 'x' : binstr[31-ith];
        set(header, ith, ch);
    }
    
    return header;
}

string HSA::stringify(const dynbitset &header)
{
    string str;
    for(size_t ith = 0; ith < header.size() / 2; ith++) {
        str = get(header, ith) + str;   // little endian
    }

    return str;
}
