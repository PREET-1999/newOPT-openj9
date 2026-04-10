#ifndef LOAD_STMT_H
#define LOAD_STMT_H

#include "optimizer/preetAnalysis/statements/Statement.hpp"

class LoadStmt : public Statement {

public:
    LoadStmt(TR::TreeTop *tt,StatementInfoTable* stmtInfo);

   
    // Override Gen()
    virtual PTG* Gen();    
    virtual PTG* Kill();

    virtual PTG* SetDiff(PTG *kill);
    virtual PTG* SetUnion(PTG *filteredSet, PTG *newSet);

    std::set<TR::Node*> getNodePointedByBase();
    // std::set<TR::Node*> getNodeToBeStoredIntoBase();
    TR::SymbolReference* getSymRef();
    //function to get the slot for lhs auto variable 
    int getlhsAuto(); // a->auto a=b.f
        int getrhsAuto(); //b->auto  a =b.f

};

#endif
