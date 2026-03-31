#include "StoreStmt.hpp"
#include "il/PTG.hpp"
#include "il/Node.hpp"
#include "il/Node_inlines.hpp"
#include <iostream>
#include <bits/stdc++.h>
#include "il/SymbolReference.hpp"

StoreStmt::StoreStmt(TR::TreeTop *tt)
{
    _tt = tt;
}

PTG *StoreStmt::Gen()
{
    // create a new PTG object (stack and heap)
    PTG *genPTG = new PTG();
    std::cout << "In store stmt gen" << "\n";
    std::set<TR::Node*> baseNode = getNodePointedByBase();
    std::set<TR::Node*> storedNode =getNodeToBeStoredIntoBase();
    TR::SymbolReference* f = getSymRef();

    std::cout<<"genPTG heap before inserting...\n";
    genPTG->printHeap();

    //inserting into heap [baseNode,f]={storedNode}
    for(auto a : baseNode){
        for(auto b : storedNode){
            /*
                SymbolReference* f = new SymbolReference("f");
                pair<Node*,SymbolReference*> NodeObjectField = {n3,f};
            */
           std::pair<TR::Node*,TR::SymbolReference*> NodeObjectField = {a,f};
            genPTG->insertIntoHeap(NodeObjectField,b);
        }
    }
    std::cout<<"genPTG heap....\n";
    genPTG->printHeap();


    //======> lhs.f=rhs
    // 1. get the set<Node*> of the [auto of lhs] present in _stack
    // 2. check which symbol reference will you store (of which node? ... maybe its awrtbar's but still check )
    // 3. get the set<Node*>  of [auto of rhs] present in _stack

    return genPTG;
}

PTG *StoreStmt::Kill()
{
    std::cout << "in kill the initial inset is\n";

    PTG *killPTG = new PTG();

    return killPTG;
}

PTG *StoreStmt::SetDiff(PTG *kill)
{

    std::cout << "SetDiff trial\n";

    PTG *copyIn = new PTG();
    copyIn->_stack = _tt->_in->_stack;
    copyIn->_heap = _tt->_in->_heap;
    return copyIn;
}

PTG *StoreStmt::SetUnion(PTG *filteredSet, PTG *newSet)
{

    PTG *out = new PTG();
    // I guess we dont need the filteredSet, so we can change it in place
    // but as of now just create a copy , to maybe debug the filteredSet if needed
    out->_stack = filteredSet->_stack;
    out->_heap = filteredSet->_heap;
    std::map<std::pair<TR::Node *, TR::SymbolReference *>, std::set<TR::Node *>>::iterator it;
    std::set<TR::Node *>::iterator nodeIt;
    std::pair<TR::Node *,TR::SymbolReference*> objectFieldPair;
    std::set<TR::Node *> nodes;
    for (it = newSet->_heap.begin(); it != newSet->_heap.end(); ++it)
        {
            objectFieldPair = it->first;
            nodes = it->second;
                    
            
            // out->_heap.insert(std::pair<std::pair<TR::Node *, TR::SymbolReference *>, std::set<TR::Node *>>(objectFieldPair,nodes));
            for(auto node : nodes){
                out->insertIntoHeap(objectFieldPair,node);
            }
        }
    // out->printStack();
    // out->printHeap();
    return out;
}

std::set<TR::Node *> StoreStmt::getNodePointedByBase()
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
                std::cout << "slot for Store(base)is " << slot << "\n";
                nodes = _tt->_in->getNodeSetForKeyInStack(slot);
                for (TR::Node *node : nodes)
                {
                    std::cout << node << std::endl; // prints the pointer address
                }
            }
        }
    }
    return nodes;
}

std::set<TR::Node *> StoreStmt::getNodeToBeStoredIntoBase()
{
    std::set<TR::Node *> nodes ; //a.f=b   b->{0xab , ...}
    TR::Node *node = _tt->getNode();
    TR::Node *firstChildNode = node->getFirstChild(); // since nullcheck would be its parent
    if (firstChildNode)
    {
        TR::Node *storeNode = firstChildNode;
        TR::Node *baseNode = storeNode->getFirstChild(); // base Node of store stmt
        TR::Node *storingValueNode = storeNode->getSecondChild(); //rhs node of store stmt

        if (storingValueNode->getOpCode().hasSymbolReference() && storingValueNode->getSymbolReference())
        {
            TR::SymbolReference *symRef = storingValueNode->getSymbolReference();
            TR::Symbol *sym = symRef->getSymbol();
            if (sym->getKind() == TR::Symbol::IsAutomatic)
            {
                int32_t slot = symRef->getCPIndex();
                std::cout << "slot for Store(rhs)is " << slot << "\n";
                nodes = _tt->_in->getNodeSetForKeyInStack(slot);
                for (TR::Node *node : nodes)
                {
                    std::cout << node << std::endl; // prints the pointer address
                }
            }
        }
    }
    return nodes;
}

TR::SymbolReference *StoreStmt::getSymRef()
{
     TR::SymbolReference *symRef = nullptr;

     TR::Node *node = _tt->getNode();
    TR::Node *firstChildNode = node->getFirstChild(); // since nullcheck would be its parent
    if (firstChildNode) //the awrtbari 
    {
        symRef = firstChildNode->getSymbolReference();
        int32_t index = symRef->getCPIndex();
        std::cout<<"(store)symRef " <<symRef <<"\n";
        std::cout<<"index " <<index <<"\n";

    }

    return symRef;
}
