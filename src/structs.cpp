#include "structs.hpp"
#include "hsa.hpp"

Rule::Rule(int rid, int sid, string prefix, int in_port, int out_port, int priority) :
    rid(rid), sid(sid), prefix(prefix), in_port(in_port), out_port(out_port), priority(priority)
{
    in_header = HSA::translate(prefix);
    out_header = getAvailableOutHeader(in_header);
}

int Rule::getSID()
{
    return sid;
}
    
int Rule::getInPort()
{
    return in_port;
}

int Rule::getOutPort()
{
    return out_port;
}
     
dynbitset Rule::getInHeader()
{
    return in_header;
}
    
dynbitset Rule::getOutHeader()
{
    return out_header;
}

dynbitset Rule::getAvailableOutHeader(dynbitset &available_in_header)
{
    return available_in_header;
}

RuleNode::RuleNode(int rid, int sid, string prefix, int in_port, int out_port, int priority) :
    rule(Rule(rid, sid, prefix, in_port, out_port, priority))
{

}

Rule& RuleNode::getRule()
{
    return rule;
}

vector<int>& RuleNode::getNexts()
{
    return nexts;
}

void RuleNode::addNext(int rid)
{
    nexts.push_back(rid);
}

void RuleNode::addNextIfNotExists(int rid)
{
    for(auto r : nexts) {
        if(r == rid) {
            return;
        }
    }
    
    addNext(rid);
}

SwitchNode::SwitchNode(int sid, int sidx) :
    sid(sid), sidx(sidx)
{

}
    
int SwitchNode::getSID()
{
    return sid;
}

int SwitchNode::getSIdx()
{
    return sidx;
}

vector<int>& SwitchNode::getNeighbors()
{
    return neighbors;
}

vector<int>& SwitchNode::getRules()
{
    return rules;
}

void SwitchNode::addNeighbor(int sid)
{
    neighbors.push_back(sid);
}
   
void SwitchNode::addRule(int rid)
{
    rules.push_back(rid);
}


