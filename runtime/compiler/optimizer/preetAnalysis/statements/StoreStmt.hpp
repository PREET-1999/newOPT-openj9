#ifndef STORE_STMT_H
#define STORE_STMT_H

#include "optimizer/preetAnalysis/statements/Statement.hpp"

class StoreStmt : public Statement {

public:
    StoreStmt(TR::TreeTop *tt);

   
    // Override Gen()
    virtual PTG* Gen();    
    virtual PTG* Kill();

    virtual PTG* SetDiff(PTG *kill);
    virtual PTG* SetUnion(PTG *filteredSet, PTG *newSet);

    std::set<TR::Node*> getNodePointedByBase();
    std::set<TR::Node*> getNodeToBeStoredIntoBase();
    TR::SymbolReference* getSymRef();

    int getlhsAuto(); //a  for a.f=b
    int getrhsAuto(); //b for a.f=b

};

#endif
