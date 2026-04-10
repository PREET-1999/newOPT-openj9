#ifndef UNKNOWN_STMT_H
#define UNKNOWN_STMT_H

#include "optimizer/preetAnalysis/statements/Statement.hpp"

class UnknownStmt : public Statement {

public:
    UnknownStmt(TR::TreeTop *tt,StatementInfoTable* stmtInfo);

   
    // Override Gen()
    virtual PTG* Gen();    
    virtual PTG* Kill();

    virtual PTG* SetDiff(PTG *kill);
    virtual PTG* SetUnion(PTG *filteredSet, PTG *newSet);



};

#endif
