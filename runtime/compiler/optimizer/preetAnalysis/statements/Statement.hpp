#ifndef STATEMENT_H
#define STATEMENT_H
#include "il/PTG.hpp"
#include "il/TreeTop.hpp"
class StatementInfoTable;

class Statement{
    public:
    TR::TreeTop* _tt;
    StatementInfoTable* _stmtInfo;
    virtual PTG* Gen()=0;
    virtual PTG* Kill()=0;
    virtual PTG* SetDiff(PTG *kill)=0;
    virtual PTG* SetUnion(PTG *filteredSet, PTG *newSet)=0;
    PTG * KillSetPostStrongUpdate(int searchKey, PTG *inSet);


};


#endif