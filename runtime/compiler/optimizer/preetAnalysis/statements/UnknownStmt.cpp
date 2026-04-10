#include "UnknownStmt.hpp"
UnknownStmt::UnknownStmt(TR::TreeTop *tt,StatementInfoTable* stmtInfo)
{
    _tt = tt;
    _stmtInfo = stmtInfo;
}

PTG *UnknownStmt::Gen()
{
        PTG *genPTG = new PTG();
    return genPTG;

}

PTG *UnknownStmt::Kill()
{
        PTG *killPTG = new PTG();
    return killPTG;

}

PTG *UnknownStmt::SetDiff(PTG *kill)
{
    PTG *copyIn = new PTG();
    copyIn->_stack = _tt->_in->_stack;
    copyIn->_heap = _tt->_in->_heap;
    return copyIn;

}

PTG *UnknownStmt::SetUnion(PTG *filteredSet, PTG *newSet)
{
        // I guess we dont need the filteredSet, so we can change it in place
    // but as of now just create a copy , to maybe debug the filteredSet if needed
    PTG *out = new PTG();
    out->_stack = filteredSet->_stack;
    out->_heap = filteredSet->_heap;

    std::map<int, std::set<TR::Node*>>::iterator it;
    std::set<TR::Node*>::iterator nodeIt;
    for (it = newSet->_stack.begin(); it != newSet->_stack.end(); ++it)
        {
            out->_stack.insert(std::pair<int,std::set<TR::Node*>>(it->first,it->second));
        }
    // out->printStack();
    // out->printHeap();
    return out;

}
