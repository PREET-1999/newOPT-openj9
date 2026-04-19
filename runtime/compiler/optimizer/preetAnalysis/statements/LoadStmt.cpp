#include <iostream>
#include "LoadStmt.hpp"
#include "il/PTG.hpp"
#include "il/Node.hpp"
#include "il/Node_inlines.hpp"
#include <bits/stdc++.h>
#include "il/SymbolReference.hpp"
#include "optimizer/preetAnalysis/StatementInfoTable.hpp"
LoadStmt::LoadStmt(TR::TreeTop *tt, StatementInfoTable *stmtInfo)
{
    _tt = tt;
    _stmtInfo = stmtInfo;
}

PTG *LoadStmt::Gen()
{
    PTG *genPTG = new PTG();

    // a = b.[f]*
    /*
    we can form these cases
    - a is lhsBase, b is rhsBase

    - Is lhsBase a param
    - Is lhsBase pointing to bottom
    - Is lhsBase poining to actual obj

    - Is rhsBase this
    - Is rhsBase param
    - Is rhsBase pointing to bottom
    - Is rhsBase.f..(n) or toNodes tak any of it pointing to bottom
    - Is rhsBase.f..(n) or toNodes poining to actual obj

    */
    {
        int lhsAuto = getlhsAuto();
        int rhsAuto = getrhsAuto();

        if (_stmtInfo->isParam(lhsAuto))
        {
            // do nothing
            return genPTG;
        }

        if (_tt->_in->isPointsToOfKeyInStackBottom(lhsAuto))
        {
            // do nothing
            return genPTG;
        }
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

        std::set<TR::Node *> objsOfBase = getNodePointedByBase();
        std::vector<TR::Node *> toNodes;

        for (auto node : objsOfBase)
        {
            if (_stmtInfo->rhsFieldStack.size() == 0)
            {
                toNodes.push_back(node);
            }
            else
            {
                _tt->_in->findNodes(node, _stmtInfo->rhsFieldStack, 0, _stmtInfo->rhsFieldStack.size() - 1, toNodes);
            }
        }

        // Is rhsBase.f..(n) or toNodes tak any of it pointing to bottom
        for (auto node : toNodes)
        {
            if (node == nullptr)
            {
                TR::Node *bottom = nullptr;
                genPTG->insertIntoStack(lhsAuto, bottom);
                return genPTG;
            }
        }

        TR::SymbolReference *symref = getSymRef();
        for (auto node : toNodes)
        {
            genPTG->insertIntoStack(lhsAuto, node);
        }
        return genPTG;
    }

    // std::cout << "Load Gen\n";
    // // c = a.f
    // //  auto 4 = auto 1.f

    // // first get the set<Node*>pointed by 1
    // int lhsAuto = getlhsAuto();
    // int rhsAuto = getrhsAuto();

    // /*
    // Handling this and param cases before proceeding further
    // */

    // if (lhsAuto == -1) // its a param (this=b.f -> wrong   'this' isnt alowed to be assigned)
    // {
    //     return genPTG;
    // }

    // if (rhsAuto == -1)
    // {
    //     TR::Node *bottom = nullptr;
    //     genPTG->insertIntoStack(lhsAuto, bottom);

    //     // no further processing needed
    //     return genPTG;
    // }
    // /*
    //     Done handling this and param cases
    // */

    // // check if b->bottom ? (a=b.f)
    // bool isObjPointedByRhsBottom = _tt->_in->isPointsToOfKeyInStackBottom(rhsAuto);
    // if (isObjPointedByRhsBottom)
    // {
    //     // set lhs to also point to bottom
    //     TR::Node *bottom = nullptr;
    //     genPTG->insertIntoStack(lhsAuto, bottom);

    //     // no further processing needed
    //     return genPTG;
    // }

    // std::set<TR::Node *> nodeSetOfRhs = _tt->_in->getNodeSetForKeyInStack(rhsAuto);
    // for (auto node : nodeSetOfRhs)
    // {
    //     if (_tt->_in->doesStarFieldFromNodeExists(node))
    //     {
    //         TR::Node *bottom = nullptr;
    //         genPTG->insertIntoStack(lhsAuto, bottom);
    //         // no further processing
    //         return genPTG;
    //     }
    // }

    // // std::set<TR::Node*> objsOfBase = _tt->_in->getNodeSetForKeyInStack(base);
    // std::set<TR::Node *> objsOfBase = getNodePointedByBase();
    // std::vector<TR::Node *> toNodes;

    // for (auto node : objsOfBase)
    // {
    //     if (_stmtInfo->rhsFieldStack.size() == 0)
    //     {
    //         toNodes.push_back(node);
    //     }
    //     else
    //     {
    //         _tt->_in->findNodes(node, _stmtInfo->rhsFieldStack, 0, _stmtInfo->rhsFieldStack.size() - 1, toNodes);
    //     }
    // }

    // // if to Nodes has bottom , point lhs to bottom
    // for (auto node : toNodes)
    // {
    //     if (node == nullptr)
    //     {
    //         TR::Node *bottom = nullptr;
    //         genPTG->insertIntoStack(lhsAuto, bottom);
    //         return genPTG;
    //     }
    // }
    // TR::SymbolReference *symref = getSymRef();
    // std::cout << "load " << symref << "\n";
    // for (auto node : toNodes)
    // {

    //     genPTG->insertIntoStack(lhsAuto, node);
    // }
    // genPTG->printStack();
    // genPTG->printHeap();

    return genPTG;
}

PTG *LoadStmt::Kill()
{
    std::cout << "in kill the initial inset is\n";

    // call the to-be made API to get auto
    int autoLHS = getlhsAuto();

    PTG *killPTG = KillSetPostStrongUpdate(autoLHS, _tt->_in);
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

PTG *LoadStmt::SetUnion(PTG *filteredSet, PTG *newSet)
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

std::set<TR::Node *> LoadStmt::getNodePointedByBase()
{
    int rhsSlot = getrhsAuto();
    std::set<TR::Node *> nodes; // base->{0xab , ...}
    nodes = _tt->_in->getNodeSetForKeyInStack(rhsSlot);
    for (TR::Node *node : nodes)
    {
        std::cout << node << std::endl; // prints the pointer address
    }

    // TR::Node *node = _tt->getNode();
    // TR::Node *firstChildNode = node->getFirstChild(); // since nullcheck would be its parent
    // if (firstChildNode)
    // {
    //     TR::Node *storeNode = firstChildNode;
    //     TR::Node *baseNode = storeNode->getFirstChild(); // base Node of store stmt
    //     if (baseNode->getOpCode().hasSymbolReference() && baseNode->getSymbolReference())
    //     {
    //         TR::SymbolReference *symRef = baseNode->getSymbolReference();
    //         TR::Symbol *sym = symRef->getSymbol();
    //         if (sym->getKind() == TR::Symbol::IsAutomatic)
    //         {
    //             int32_t slot = symRef->getCPIndex();
    //             std::cout << "slot for Load(base)is " << slot << "\n";
    //             nodes = _tt->_in->getNodeSetForKeyInStack(slot);
    //             for (TR::Node *node : nodes)
    //             {
    //                 std::cout << node << std::endl; // prints the pointer address
    //             }
    //         }
    //     }
    // }
    return nodes;
}
// this should return symRef from "base" (NOT just the first child)

TR::SymbolReference *LoadStmt::getSymRef()
{
    TR::SymbolReference *symRef = nullptr;
    int rhsStackFieldLength = _stmtInfo->rhsFieldStack.size();
    symRef = _stmtInfo->rhsFieldStack[rhsStackFieldLength - 1];

    // works for basic a = b.f
    //  TR::Node *node = _tt->getNode();
    //  TR::Node *firstChildNode = node->getFirstChild(); // since nullcheck would be its parent
    //  if (firstChildNode)                               // the aloadi
    //  {
    //      symRef = firstChildNode->getSymbolReference();
    //      int32_t index = symRef->getCPIndex();
    //      std::cout << "(load)symRef " << symRef << "\n";
    //      std::cout << "index " << index << "\n";
    //  }

    return symRef;
}

int LoadStmt::getlhsAuto()
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
    //             std::cout << "(load Lhs)slot is " << slot << "\n";
    //             return slot;
    //         }
    //     }
    // }
    // return -1;
}

int LoadStmt::getrhsAuto()
{
    return _stmtInfo->rhsAuto;

    // TR::Node *node = _tt->getNode();
    // TR::Node *firstChildNode = node->getFirstChild(); // since nullcheck would be its parent
    // if (firstChildNode)
    // {
    //     TR::Node *storeNode = firstChildNode;
    //     TR::Node *baseNode = storeNode->getFirstChild(); // base Node of store stmt
    //     if (baseNode->getOpCode().hasSymbolReference() && baseNode->getSymbolReference())
    //     {
    //         TR::SymbolReference *symRef = baseNode->getSymbolReference();
    //         TR::Symbol *sym = symRef->getSymbol();
    //         if (sym->getKind() == TR::Symbol::IsAutomatic)
    //         {
    //             int32_t slot = symRef->getCPIndex();
    //             std::cout << "(load Rhs)slot is " << slot << "\n";
    //             return slot;
    //         }
    //     }
    // }
    // return -1;
}
