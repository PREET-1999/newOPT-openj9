#include<iostream>
#include "LoadStmt.hpp"
#include "il/PTG.hpp"
#include "il/Node.hpp"
#include "il/Node_inlines.hpp"
#include <bits/stdc++.h>
#include "il/SymbolReference.hpp"
LoadStmt::LoadStmt(TR::TreeTop *tt)
{
    _tt = tt;
}

PTG *LoadStmt::Gen()
{
     PTG *genPTG = new PTG();

    std::cout<<"Load Gen\n";
    //c = a.f
//  auto 4 = auto 1.f 

    //first get the set<Node*>pointed by 1
    int lhsAuto = getAuto(); //actuallly this will not be in this tt, figure out how to deal with this
    // std::set<TR::Node*> objsOfBase = _tt->_in->getNodeSetForKeyInStack(base);
    std::set<TR::Node*> objsOfBase = getNodePointedByBase();

    TR::SymbolReference* symref = getSymRef();
    std::cout<<"load " <<symref <<"\n";
    for(auto node : objsOfBase){
        // cout<<"node" <<node->_id <<"\n";
        
        //for this node , find the nodes pointed to by its f(symRef) field
        std::set<TR::Node*> nodeSet = _tt->_in->getNodeSetForKeyInHeap(std::pair<TR::Node*, TR::SymbolReference*>{node, symref});
        for(auto heapNode : nodeSet){
            // cout<<"O" <<heapNode->_id <<" ";
            genPTG->insertIntoStack(lhsAuto,heapNode);

        }
        std::cout<<"\n";
    }
    genPTG->printStack();
    genPTG->printHeap();

    return genPTG;
}

PTG *LoadStmt::Kill()
{
    std::cout << "in kill the initial inset is\n";

    // call the to-be made API to get auto
    int autoLHS = getAuto();

    PTG* killPTG = KillSetPostStrongUpdate(autoLHS, _tt->_in);
    // cout<<"ObjAllocatuion KILL\n";
    // killPTG->printStack();
    // killPTG->printHeap();

    return killPTG;
}

PTG *LoadStmt::SetDiff(PTG *kill)
{

    std::cout << "SetDiff trial\n";

    PTG *copyIn = new PTG();
    copyIn->_stack = _tt->_in->_stack;
    copyIn->_heap = _tt->_in->_heap;
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

PTG *LoadStmt::SetUnion(PTG *filteredSet, PTG *newSet)
{

    PTG *out = new PTG();
    // I guess we dont need the filteredSet, so we can change it in place
    // but as of now just create a copy , to maybe debug the filteredSet if needed
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

std::set<TR::Node *> LoadStmt::getNodePointedByBase()
{
    std::set<TR::Node *> nodes ; //base->{0xab , ...}
    TR::Node *node = _tt->getNode();
    TR::Node *firstChildNode = node->getFirstChild(); // since nullcheck would be its parent
    if (firstChildNode)
    {
        TR::Node *storeNode = firstChildNode;
        TR::Node *baseNode = storeNode->getFirstChild(); // base Node of store stmt
        if (baseNode->getOpCode().hasSymbolReference() && baseNode->getSymbolReference())
        {
            TR::SymbolReference *symRef = baseNode->getSymbolReference();
            TR::Symbol *sym = symRef->getSymbol();
            if (sym->getKind() == TR::Symbol::IsAutomatic)
            {
                int32_t slot = symRef->getCPIndex();
                std::cout << "slot for Load(base)is " << slot << "\n";
                nodes = _tt->_in->getNodeSetForKeyInStack(slot);
                for (TR::Node *node : nodes)
                {
                    std::cout << node << std::endl; // prints the pointer address
                }
            }
        }
    }
    return nodes;}

TR::SymbolReference *LoadStmt::getSymRef()
{
     TR::SymbolReference *symRef = nullptr;

     TR::Node *node = _tt->getNode();
    TR::Node *firstChildNode = node->getFirstChild(); // since nullcheck would be its parent
    if (firstChildNode) //the aloadi 
    {
        symRef = firstChildNode->getSymbolReference();
        int32_t index = symRef->getCPIndex();
        std::cout<<"(load)symRef " <<symRef <<"\n";
        std::cout<<"index " <<index <<"\n";

    }

    return symRef;
}

int LoadStmt::getAuto()
{
    TR::Node *node = _tt->getNode();
    if (node->getOpCodeValue() == TR::astore)
    {
        if (node->getOpCode().hasSymbolReference() && node->getSymbolReference()){
            TR::SymbolReference *symRef = node->getSymbolReference();
            TR::Symbol *sym = symRef->getSymbol();
            if( sym->getKind() == TR::Symbol::IsAutomatic){
                int32_t slot = symRef->getCPIndex();
                std::cout<<"(load lhs)slot is " <<slot <<"\n";
                return slot;
            }
        }
    }
    return -1; 
}