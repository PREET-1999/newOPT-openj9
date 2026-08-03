#include "optimizer/preetAnalysis/statements/ObjectAllocationStmt.hpp"
#include "il/PTG.hpp"
#include "il/Node.hpp"
#include "il/Node_inlines.hpp"
#include <iostream>
#include <bits/stdc++.h>
#include "ObjectAllocationStmt.hpp"
#include "il/SymbolReference.hpp"

// using namespace std;
ObjectAllocationStmt::ObjectAllocationStmt(TR::TreeTop *tt,StatementInfoTable* stmtInfo)
{
    _tt = tt;
    _stmtInfo=stmtInfo;
}

int ObjectAllocationStmt::getAuto()
{
    TR::Node *node = _tt->getNode();
    if (node->getOpCodeValue() == TR::astore)
    {
        if (node->getOpCode().hasSymbolReference() && node->getSymbolReference()){
            TR::SymbolReference *symRef = node->getSymbolReference();
            TR::Symbol *sym = symRef->getSymbol();
            if( sym->getKind() == TR::Symbol::IsAutomatic){
                int32_t slot = symRef->getCPIndex();

                if(sym->isLocalObject())
                     std::cout<<"<auto " <<symRef->getCPIndex() <<"> is local\n"; 

                std::cout<<"slot is " <<slot <<"\n";
                return slot;
            }
        }
    }
    return 0;
}

TR::Node *ObjectAllocationStmt::getNewNode()
{   
    TR::Node *node = _tt->getNode();
    TR::Node *firstChildNode = node->getFirstChild();
    return firstChildNode;
}


PTG *ObjectAllocationStmt::Gen()
{
    // create a new PTG object (stack and heap)
    PTG *genPTG = new PTG();

    // //call the to-be made API to get auto and Node * TODO
    // TR::Node* newNode = new Node(6);
    // int autoLHS = 1; //1 will actually be whatever will be the auto int

    // genPTG->insertIntoStack(autoLHS, newNode);
    // cout<<"ObjAllocatuion GEN\n";
    // genPTG->printStack();
    // genPTG->printHeap();

    int autoLHS = getAuto();
    TR::Node* newNode = getNewNode();
    std::cout<<"[New]node " <<newNode <<"\n";
    genPTG->insertIntoStack(autoLHS, newNode);

    std::cout<<" [ NEW : GEN ]\n";
    genPTG->printStack();
    genPTG->printHeap();
    std::cout<<" [ ---NEW : GEN----- ]\n";


    return genPTG;
}

PTG *ObjectAllocationStmt::Kill()
{
    std::cout << "in kill the initial inset is\n";
    _tt->_in->printStack();
    _tt->_in->printHeap();

    // call the to-be made API to get auto
    int autoLHS = getAuto();

    PTG* killPTG = KillSetPostStrongUpdate(autoLHS, _tt->_in);
    // cout<<"ObjAllocatuion KILL\n";
    // killPTG->printStack();
    // killPTG->printHeap();

       std::cout<<" [ NEW : KILL ]\n";
    killPTG->printStack();
    killPTG->printHeap();
    std::cout<<" [ ---NEW : KILL----- ]\n";

    return killPTG;

}

PTG *ObjectAllocationStmt::SetDiff(PTG *kill)
{

    std::cout << "SetDiff trial\n";
    // new ke isme just
    //  lets check by seeing if copy is working as expected
    PTG *copyIn = new PTG();
    copyIn->_stack = _tt->_in->_stack;
    copyIn->_heap = _tt->_in->_heap;

    // copyIn->printStack();

    // cout<<"CopyIn ";
    // copyIn->printStack();
    // cout<<"tt";
    // _tt->_in->printStack();

    // //actual delete : kill content from copy In and return
    // //can we move this to a routine of stack?
    // //1.iterate over kill stack keys and remove from copyIn if present
    std::map<int, std::set<TR::Node*>>::iterator it;
    std::set<TR::Node*>::iterator nodeIt;
    for (it = kill->_stack.begin(); it != kill->_stack.end(); ++it)
        {
            std::map<int, std::set<TR::Node*>>::iterator keyFoundIt;
            keyFoundIt = copyIn->_stack.find(it->first);
            if(keyFoundIt != copyIn->_stack.end()){
                // cout<<"remove " <<it->first <<"?\n";
                copyIn->_stack.erase(keyFoundIt);
            }
        }
    // copyIn->printStack();
    return copyIn;
}

PTG *ObjectAllocationStmt::SetUnion(PTG *filteredSet, PTG *newSet)
{

    // I guess we dont need the filteredSet, so we can change it in place
    // but as of now just create a copy , to maybe debug the filteredSet if needed
    PTG *out = new PTG();
    out->_stack = filteredSet->_stack;
    out->_heap = filteredSet->_heap;

    std::map<int, std::set<TR::Node*>>::iterator it;
    std::set<TR::Node*>::iterator nodeIt;
    std::set<TR::Node *> nodes;
    int autoSlot;
    for (it = newSet->_stack.begin(); it != newSet->_stack.end(); ++it)
    {
        autoSlot = it->first;
        nodes = it->second;
        for (auto node : nodes)
        {
            out->insertIntoStack(autoSlot,node);
        }
    }
    // out->printStack();
    // out->printHeap();
    return out;
}
