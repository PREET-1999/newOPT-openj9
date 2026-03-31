#ifndef CALL_STMT_H
#define CALL_STMT_H

#include "optimizer/preetAnalysis/statements/Statement.hpp"
#include <vector>
enum class MethodCallKind;
class CallStmt : public Statement {

public:
    CallStmt(TR::TreeTop *tt);

   
    // Override Gen()
    virtual PTG* Gen();    
    virtual PTG* Kill();

    virtual PTG* SetDiff(PTG *kill);
    virtual PTG* SetUnion(PTG *filteredSet, PTG *newSet);

    MethodCallKind kindOfMethodCall();
    char * methodCallName();

    //as of now kept void to just inspect arguments of call, would modify accordingly
    std::vector<TR::Node*> inspectCallArguments();

    //getting slots for a given node (here argument node)
    int getArgumentAuto(TR::Node* argNode);
    //getting slot for lhs of call
    int getAutoSlotIfNonVoidReturn(TR::Node* node);
};


#endif
