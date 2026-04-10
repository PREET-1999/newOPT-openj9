#ifndef OBJECT_ALLOCATION_STMT_H
#define OBJECT_ALLOCATION_STMT_H

#include "optimizer/preetAnalysis/statements/Statement.hpp"

class ObjectAllocationStmt : public Statement {

public:
    ObjectAllocationStmt(TR::TreeTop *tt,StatementInfoTable *stmtInfo);

    //function to get the slot for lhs auto variable 
    int getAuto();
    TR::Node* getNewNode();
    // Override Gen()
    virtual PTG* Gen();    
    virtual PTG* Kill();

    virtual PTG* SetDiff(PTG *kill);
    virtual PTG* SetUnion(PTG *filteredSet, PTG *newSet);



};

#endif
