#ifndef COPY_STMT_H
#define COPY_STMT_H

#include "optimizer/preetAnalysis/statements/Statement.hpp"

class CopyStmt : public Statement {

public:
    CopyStmt(TR::TreeTop *tt,StatementInfoTable* stmtInfo);

   
    // Override Gen()
    virtual PTG* Gen();    
    virtual PTG* Kill();

    virtual PTG* SetDiff(PTG *kill);
    virtual PTG* SetUnion(PTG *filteredSet, PTG *newSet);

    std::set<TR::Node*> getNodePointedByBase();
    // std::set<TR::Node*> getNodeToBeStoredIntoBase();
    TR::SymbolReference* getSymRef();
    //function to get the slot for lhs auto variable 
    int getFromAuto();
    int getToAuto();

};

#endif
