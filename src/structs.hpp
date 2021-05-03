#ifndef STRUCTS_H
#define STRUCTS_H

//#define DEBUG
//#define VERBOSE

#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <boost/dynamic_bitset.hpp>

using namespace std;

#define dynbitset boost::dynamic_bitset<>

#define MAX_NUM_SWITCH 1000
#define INF (numeric_limits<int>::max())
#define SID_OF_MASKLEN -1
#define PORT_HOST 1000

class RuleNode;
class SwitchNode;

// rule id -> rule node
typedef unordered_map<int, RuleNode> RuleGraph;

// switch id -> switch node
typedef unordered_map<int, SwitchNode> SwitchGraph;

// swith id -> vector<switch id>
typedef unordered_map<int, set<int>> SwitchGraphAlpha;
typedef unordered_map<int, set<int>> SwitchGraphBeta;

// help to build beta
typedef int SwitchGraphMatrix[MAX_NUM_SWITCH][MAX_NUM_SWITCH];

// path in transitive closure
typedef unordered_map<int, unordered_map<int, int>> TransPath;

// rule id -> available header in trasitive closure and maximum matching
typedef unordered_map<int, dynbitset> HeaderMap;

// non-disjoint rule path set finally found
typedef vector<vector<int>> PathSet;

// targets set derived from rule path
typedef set<vector<int>> TargetsSet;

// equivalence partition in phase one
typedef set<set<int>> Partition; 

// report header <maskbit, value>
typedef pair<int, bool> Color;

// maskbit -> vector<switch id>
typedef unordered_map<int, vector<int>> Coloring;

// <maskbit, value> -> vector<switch id>
typedef map<Color, vector<int>> CompactColoring;

// report header assignment <sid, <maskbit, value>>
typedef unordered_map<int, Color> Assignments;

// switch id -> test header for per-rule test
typedef unordered_map<int, dynbitset> SwitchTestHeaders;

// rule path -> <packet header, test header> (for non-single path)
typedef map<vector<int>, dynbitset> PathPacketHeaders;
typedef map<vector<int>, dynbitset> PathTestHeaders;

class Rule {
public:
    Rule()=default;
    Rule(int rid, int sid, string prefix, int in_port, int out_port, int priority);

    // getter
    int getSID();
    int getInPort();
    int getOutPort();
     
    dynbitset getInHeader();
    dynbitset getOutHeader();
    dynbitset getAvailableOutHeader(dynbitset &available_in_header);

private:
    int rid;
    int sid;
    string prefix;
    int in_port;
    int out_port;
    int priority;
    
    // split header into in and out to support set-field
    dynbitset in_header;
    dynbitset out_header;
};

class RuleNode {
public:
    RuleNode()=default;
    RuleNode(int rid, int sid, string prefix, int in_port, int out_port, int priority);

    // getter
    Rule& getRule();
    vector<int>& getNexts();
    
    // setter
    void addNext(int rid);
    void addNextIfNotExists(int rid);

private:
    Rule rule;
    vector<int> nexts;
};

class SwitchNode {
public:
    SwitchNode()=default;
    SwitchNode(int sid, int sidx);

    // getter
    int getSID();
    int getSIdx();
    vector<int>& getNeighbors();
    vector<int>& getRules();

    // setter
    void addNeighbor(int sid);
    void addRule(int rid);

private:
    int sid;
    int sidx;
    vector<int> neighbors;
    vector<int> rules;
};

#endif

