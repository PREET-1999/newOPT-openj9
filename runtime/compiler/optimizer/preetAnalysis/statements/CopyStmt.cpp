#include <iostream>
#include "CopyStmt.hpp"
#include "il/PTG.hpp"
#include "il/Node.hpp"
#include "il/Node_inlines.hpp"
#include <bits/stdc++.h>
#include "il/SymbolReference.hpp"
#include "optimizer/preetAnalysis/StatementInfoTable.hpp"

CopyStmt::CopyStmt(TR::TreeTop *tt, StatementInfoTable *stmtInfo)
{
    _tt = tt;
    _stmtInfo = stmtInfo;
}

PTG *CopyStmt::Gen()
{
    PTG *genPTG = new PTG();

    std::cout << "Copy Gen\n";
    // c = a

    // a = b
    /*
    we can form these cases
    - a is lhsAuto, b is rhsBase

    - Is lhsAuto a param
    - Is lhsAuto pointing to bottom
    - Is lhsAuto poining to actual obj

    - Is rhsBase this
    - Is rhsBase param
    - Is rhsBase pointing to bottom
    - Is rhsBase poining to actual obj
    */
    {
        int lhsAuto = getToAuto();
        int rhsAuto = getFromAuto();

        if (_stmtInfo->isParam(lhsAuto))
        {
            // do nothing
            return genPTG;
        }
        //This did affect other statements(copy stmt here), where initially had just commentedfor load
        //now commenting this too
        // if (_tt->_in->isPointsToOfKeyInStackBottom(lhsAuto))
        // {
        //     // do nothing
        //     return genPTG;
        // }

        // lhsAuto isnt pointing to bottom for sure( can point to nothing or an actualo obj)

        if (_stmtInfo->isThis(rhsAuto))
        {
            TR::Node *bottom = nullptr;
            genPTG->insertIntoStack(lhsAuto, bottom);
            return genPTG;
        }
        if (_stmtInfo->isParam(rhsAuto))
        {
            TR::Node *bottom = nullptr;
            genPTG->insertIntoStack(lhsAuto, bottom);
            return genPTG;
        }
        if (_tt->_in->isPointsToOfKeyInStackBottom(rhsAuto))
        {
            TR::Node *bottom = nullptr;
            genPTG->insertIntoStack(lhsAuto, bottom);
            return genPTG;
        }

        //  rhsAuto isnt pointing to bottom for sure
        std::set<TR::Node *> toNodes = _tt->_in->getNodeSetForKeyInStack(rhsAuto);
        for (auto toNode : toNodes)
        {
            genPTG->insertIntoStack(lhsAuto, toNode);
        }
    }

    // // first get the set<Node*>pointed by 1
    // int lhsAuto = getToAuto();
    // int rhsAuto = getFromAuto();

    // // If rhs is this or param
    // if (rhsAuto == -1)
    // {
    //     // set lhs to also point to bottom
    //     TR::Node *bottom = nullptr;
    //     genPTG->insertIntoStack(lhsAuto, bottom);

    //     // no further processing needed
    //     return genPTG;
    // }

    // // std::set<TR::Node*> objsOfBase = _tt->_in->getNodeSetForKeyInStack(base);
    // std::set<TR::Node *> objsOfRhsAuto = _tt->_in->getNodeSetForKeyInStack(rhsAuto);

    // for (auto node : objsOfRhsAuto)
    // {
    //     genPTG->insertIntoStack(lhsAuto, node);
    // }
    // genPTG->printStack();
    // genPTG->printHeap();

           std::cout<<" [ COPY : GEN ]\n";
    genPTG->printStack();
    genPTG->printHeap();
    std::cout<<" [ ---COPY : GEN----- ]\n";
    return genPTG;
}

PTG *CopyStmt::Kill()
{
    std::cout << "in kill the initial inset is\n";

    // call the to-be made API to get auto
    int autoLHS = getToAuto();

    PTG *killPTG = KillSetPostStrongUpdate(autoLHS, _tt->_in);
    // cout<<"ObjAllocatuion KILL\n";
    // killPTG->printStack();
    // killPTG->printHeap();


       std::cout<<" [ COPY : KILL ]\n";
    killPTG->printStack();
    killPTG->printHeap();
    std::cout<<" [ ---COPY : KILL----- ]\n";
    return killPTG;
}

PTG *CopyStmt::SetDiff(PTG *kill)
{

    std::cout << "SetDiff trial\n";

    PTG *copyIn = new PTG();
    copyIn->_stack = _tt->_in->_stack;
    copyIn->_heap = _tt->_in->_heap;

    std::map<int, std::set<TR::Node *>>::iterator it;
    std::set<TR::Node *>::iterator nodeIt;
    for (it = kill->_stack.begin(); it != kill->_stack.end(); ++it)
    {
        std::map<int, std::set<TR::Node *>>::iterator keyFoundIt;
        keyFoundIt = copyIn->_stack.find(it->first);
        if (keyFoundIt != copyIn->_stack.end())
        {
            // cout<<"remove " <<it->first <<"?\n";
            copyIn->_stack.erase(keyFoundIt);
        }
    }
    // copyIn->printStack();
    return copyIn;
}

PTG *CopyStmt::SetUnion(PTG *filteredSet, PTG *newSet)
{

    PTG *out = new PTG();
    // I guess we dont need the filteredSet, so we can change it in place
    // but as of now just create a copy , to maybe debug the filteredSet if needed
    out->_stack = filteredSet->_stack;
    out->_heap = filteredSet->_heap;
    std::map<int, std::set<TR::Node *>>::iterator it;
    std::set<TR::Node *>::iterator nodeIt;
    std::set<TR::Node *> nodes;
    int autoSlot;
    for (it = newSet->_stack.begin(); it != newSet->_stack.end(); ++it)
    {
        autoSlot = it->first;
        nodes = it->second;
        for (auto node : nodes)
        {
            out->insertIntoStack(autoSlot, node);
        }
    }
    // out->printStack();
    // out->printHeap();
    return out;
}

std::set<TR::Node *> CopyStmt::getNodePointedByBase()
{
    std::set<TR::Node *> nodes; // base->{0xab , ...}
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
                std::cout << "slot for Copy(base)is " << slot << "\n";
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

TR::SymbolReference *CopyStmt::getSymRef()
{
    TR::SymbolReference *symRef = nullptr;

    TR::Node *node = _tt->getNode();
    TR::Node *firstChildNode = node->getFirstChild(); // since nullcheck would be its parent
    if (firstChildNode)                               // the aloadi
    {
        symRef = firstChildNode->getSymbolReference();
        int32_t index = symRef->getCPIndex();
        std::cout << "(Copy)symRef " << symRef << "\n";
        std::cout << "index " << index << "\n";
    }

    return symRef;
}

int CopyStmt::getFromAuto()
{
    return _stmtInfo->rhsAuto;

    // TR::Node *node = _tt->getNode();
    // if (node->getOpCodeValue() == TR::astore)
    // {
    //     TR::Node *firstChildNode = node->getFirstChild();
    //     if (firstChildNode)
    //     {
    //         if (firstChildNode->getOpCodeValue() == TR::aload)
    //         {
    //             TR::SymbolReference *symRef = firstChildNode->getSymbolReference();
    //             TR::Symbol *sym = symRef->getSymbol();
    //             if (sym->getKind() == TR::Symbol::IsAutomatic)
    //             {
    //                 int32_t slot = symRef->getCPIndex();
    //                 std::cout << "(Copy rhs)slot is " << slot << "\n";
    //                 return slot;
    //             }
    //         }
    //     }
    // }
    // return 0;
}

int CopyStmt::getToAuto()
{
    return _stmtInfo->lhsAuto;
    // TR::Node *node = _tt->getNode();
    // if (node->getOpCodeValue() == TR::astore)
    // {
    //     if (node->getOpCode().hasSymbolReference() && node->getSymbolReference())
    //     {
    //         TR::SymbolReference *symRef = node->getSymbolReference();
    //         TR::Symbol *sym = symRef->getSymbol();
    //         if (sym->getKind() == TR::Symbol::IsAutomatic)
    //         {
    //             int32_t slot = symRef->getCPIndex();
    //             std::cout << "(Copy lhs)slot is " << slot << "\n";
    //             return slot;
    //         }
    //     }
    // }
    // return 0;
}