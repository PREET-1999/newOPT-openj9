#include "StoreStmt.hpp"
#include "il/PTG.hpp"
#include "il/Node.hpp"
#include "il/Node_inlines.hpp"
#include <iostream>
#include <bits/stdc++.h>
#include "il/SymbolReference.hpp"
#include "optimizer/preetAnalysis/StatementInfoTable.hpp"
StoreStmt::StoreStmt(TR::TreeTop *tt, StatementInfoTable *stmtInfo)
{
    _tt = tt;
    _stmtInfo = stmtInfo;
}

PTG *StoreStmt::Gen()
{
    // create a new PTG object (stack and heap)
    PTG *genPTG = new PTG();
    std::cout << "In store stmt gen" << "\n";

    int lhsAuto = getlhsAuto();
    int rhsAuto = getrhsAuto();
    /*
   Handling this and param cases before proceeding further
   */

    if (lhsAuto == -1) // its a param (this=b.f -> wrong   'this' isnt alowed to be assigned)
    {
        return genPTG;
    }

    if (rhsAuto == -1)
    {

        TR::SymbolReference *f = getSymRef();

        std::vector<TR::Node *> fromNodes;
        std::set<TR::Node *> baseNode = getNodePointedByBase();
        for (auto node : baseNode)
        {
            if (_stmtInfo->lhsFieldStack.size() == 1)
            {
                fromNodes.push_back(node);
            }
            else
            {
                _tt->_in->findNodes(node, _stmtInfo->lhsFieldStack, 0, _stmtInfo->lhsFieldStack.size() - 2, fromNodes);
            }
        }
        TR::Node *bottom = nullptr;
        for (auto fromNode : fromNodes)
        {
            std::pair<TR::Node *, TR::SymbolReference *> NodeObjectField = {fromNode, f};
            genPTG->insertIntoHeap(NodeObjectField, bottom);
        }

        // no further processing needed
        return genPTG;
    }
    /*
        Done handling this and param cases
    */

    std::set<TR::Node *> baseNode = getNodePointedByBase();
    std::set<TR::Node *> storedNode = getNodeToBeStoredIntoBase();
    TR::SymbolReference *f = getSymRef();

    std::cout << "genPTG heap before inserting...\n";
    genPTG->printHeap();

    // int lhsAuto = getlhsAuto();

    bool isObjPointedByLhsBottom = _tt->_in->isPointsToOfKeyInStackBottom(lhsAuto);
    if (isObjPointedByLhsBottom)
    {
        // no processing neeedded
        return genPTG;
    }


    std::set<TR::Node *> nodeSetOfLhs = _tt->_in->getNodeSetForKeyInStack(lhsAuto);
    for (auto node : nodeSetOfLhs)
    {
        if (_tt->_in->doesStarFieldFromNodeExists(node))
        {
            // no need to add anything to heap and no further processing
            return genPTG;
        }
    }

    std::vector<TR::Node *> fromNodes;
    std::cout << "lhs fieldStack [\n";
    for (auto field : _stmtInfo->lhsFieldStack)
    {
        int32_t index = field->getCPIndex();
        std::cout << index << " ";
    }
    std::cout << "]\n";
    for (auto node : baseNode)
    {
        if (_stmtInfo->lhsFieldStack.size() == 1)
        {
            fromNodes.push_back(node);
        }
        else
        {
            _tt->_in->findNodes(node, _stmtInfo->lhsFieldStack, 0, _stmtInfo->lhsFieldStack.size() - 2, fromNodes);
        }
    }

    std::cout << "a.f..f ki isse -> jayega ->  [ ";
    for (auto node : fromNodes)
    {
        std::cout << node << " ";
    }
    std::cout << "]\n";

    // check if bottom is present, if yes no further processing needed
    for (auto node : fromNodes)
    {
        if (node == nullptr) // can you have a routine isBOttom rather than direct nullptr
            return genPTG;
    }


    // check if b->bottom ? (a.f=b)
    bool isObjPointedByRhsBottom = _tt->_in->isPointsToOfKeyInStackBottom(rhsAuto);
    if (isObjPointedByRhsBottom)
    {

        TR::Node *bottom = nullptr;
            for (auto fromNode : fromNodes)
            {
                std::pair<TR::Node *, TR::SymbolReference *> NodeObjectField = {fromNode, f};
                genPTG->insertIntoHeap(NodeObjectField, bottom);
            }
            return genPTG;
    }



    std::vector<TR::Node *> toNodes;
    std::cout << "rhs fieldStack [\n";
    for (auto field : _stmtInfo->rhsFieldStack)
    {
        int32_t index = field->getCPIndex();
        std::cout << index << " ";
    }
    std::cout << "]\n";
    for (auto node : storedNode)
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

    // check if bottom is present, if yes "fromNode".f -> _|_
    for (auto node : toNodes)
    {

        if (node == nullptr)
        {
            TR::Node *bottom = nullptr;
            for (auto fromNode : fromNodes)
            {
                std::pair<TR::Node *, TR::SymbolReference *> NodeObjectField = {fromNode, f};
                genPTG->insertIntoHeap(NodeObjectField, bottom);
            }
            return genPTG;
        }
    }

    std::cout << "a.f..f ki ispe -> aaayega ->  [ ";
    for (auto node : toNodes)
    {
        std::cout << node << " ";
    }
    std::cout << "]\n";
    // inserting into heap [fronMode,f]={toNode}
    for (auto a : fromNodes)
    {
        TR::Symbol *sym = a->getSymbol();
        if (sym->isLocalObject())
            std::cout << "Node " << a << " is local object\n";
        for (auto b : toNodes)
        {
            /*
                SymbolReference* f = new SymbolReference("f");
                pair<Node*,SymbolReference*> NodeObjectField = {n3,f};
            */
            std::pair<TR::Node *, TR::SymbolReference *> NodeObjectField = {a, f};
            genPTG->insertIntoHeap(NodeObjectField, b);

            // checking for local allocation

            TR::Symbol *sym = b->getSymbol();
            if (sym->isLocalObject())

                std::cout << "Node " << b << " is local object\n";
        }
    }
    std::cout << "genPTG heap....\n";
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
    int rhsAuto = getrhsAuto();

    std::set<TR::Node *> storedNode = getNodeToBeStoredIntoBase();
    std::vector<TR::Node *> toNodes;
    for (auto node : storedNode)
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
    bool isObjPointedByRhsBottom = _tt->_in->isPointsToOfKeyInStackBottom(rhsAuto);
    for (auto node : toNodes)
    {

        if (node == nullptr)
        {
            isObjPointedByRhsBottom = true;
        }
    }
    if (isObjPointedByRhsBottom || _stmtInfo->rhsAuto == -1) //this or param also means rhs pointsTo Bottom
    {
        // kill previous as anyways bottom will be added
        std::set<TR::Node *> baseNodes = getNodePointedByBase();
        std::vector<TR::Node *> fromNodes;
        for (auto node : baseNodes)
        {
            if (_stmtInfo->lhsFieldStack.size() == 1)
            {
                fromNodes.push_back(node);
            }
            else
            {
                _tt->_in->findNodes(node, _stmtInfo->lhsFieldStack, 0, _stmtInfo->lhsFieldStack.size() - 2, fromNodes);
            }
        }
        TR::SymbolReference *f = getSymRef();
        for (auto fromNode : fromNodes)
        {

            std::set<TR::Node *> nodeSet = _tt->_in->getNodeSetForKeyInHeap(std::pair<TR::Node *, TR::SymbolReference *>{fromNode, f});
            for (auto node : nodeSet)
            {
                killPTG->insertIntoHeap(std::pair<TR::Node *, TR::SymbolReference *>{fromNode, f}, node);
            }
        }
    }
    std::cout << "[StoreStmt] Kill\n";
    killPTG->printHeap();
    return killPTG;
}

PTG *StoreStmt::SetDiff(PTG *kill)
{

    std::cout << "SetDiff trial\n";

    PTG *copyIn = new PTG();
    copyIn->_stack = _tt->_in->_stack;
    copyIn->_heap = _tt->_in->_heap;

    std::map<std::pair<TR::Node *, TR::SymbolReference *>, /*{01,f} */
             std::set<TR::Node *>                          /* set of Nodes */
             >::iterator it;
    std::set<TR::Node *>::iterator nodeIt;
    for (it = kill->_heap.begin(); it != kill->_heap.end(); ++it)
    {
        std::map<std::pair<TR::Node *, TR::SymbolReference *>, /*{01,f} */
                 std::set<TR::Node *>                          /* set of Nodes */
                 >::iterator keyFoundIt;
        keyFoundIt = copyIn->_heap.find(it->first);
        if (keyFoundIt != copyIn->_heap.end())
        {
            // cout<<"remove " <<it->first <<"?\n";
            copyIn->_heap.erase(keyFoundIt);
        }
    }
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
    std::pair<TR::Node *, TR::SymbolReference *> objectFieldPair;
    std::set<TR::Node *> nodes;
    for (it = newSet->_heap.begin(); it != newSet->_heap.end(); ++it)
    {
        objectFieldPair = it->first;
        nodes = it->second;

        // out->_heap.insert(std::pair<std::pair<TR::Node *, TR::SymbolReference *>, std::set<TR::Node *>>(objectFieldPair,nodes));
        for (auto node : nodes)
        {
            out->insertIntoHeap(objectFieldPair, node);
        }
    }
    // out->printStack();
    // out->printHeap();
    return out;
}

std::set<TR::Node *> StoreStmt::getNodePointedByBase()
{
    std::set<TR::Node *> nodes; // a.f =b   a->{0xab , ...}
    int lhsAuto = _stmtInfo->lhsAuto;
    nodes = _tt->_in->getNodeSetForKeyInStack(lhsAuto);

    // testing isLocalAllocation which is avaialoble is for what scenarios
    //  TR::Node *node = _tt->getNode();
    //  TR::Node *firstChildNode = node->getFirstChild(); // since nullcheck would be its parent
    //  if (firstChildNode)
    //  {
    //      TR::Node *storeNode = firstChildNode;
    //      TR::Node *baseNode = storeNode->getFirstChild(); // base Node of store stmt
    //      if (baseNode->getOpCode().hasSymbolReference() && baseNode->getSymbolReference())
    //      {
    //          TR::SymbolReference *symRef = baseNode->getSymbolReference();
    //          TR::Symbol *sym = symRef->getSymbol();
    //          if (sym->getKind() == TR::Symbol::IsAutomatic)
    //          {
    //                  if(sym->isLocalObject())
    //                   std::cout<<"<auto " <<symRef->getCPIndex() <<"> is local\n";

    //         }
    //     }
    // }

    // this works for basic a.f node
    //  std::set<TR::Node *> nodes; // a.f =b   a->{0xab , ...}
    //  int lhsAuto = getlhsAuto();
    //  nodes = _tt->_in->getNodeSetForKeyInStack(lhsAuto);

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
    //             std::cout << "slot for Store(base)is " << slot << "\n";
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

std::set<TR::Node *> StoreStmt::getNodeToBeStoredIntoBase()
{

    std::set<TR::Node *> nodes; // a.f=b   b->{0xab , ...}
    if (_stmtInfo->rhsNewNode)
        nodes.insert(_stmtInfo->rhsNewNode);
    else
    {
        int rhsAuto = _stmtInfo->rhsAuto;
        nodes = _tt->_in->getNodeSetForKeyInStack(rhsAuto);
    }

    // this works for basic a.f=b
    //  std::set<TR::Node *> nodes; // a.f=b   b->{0xab , ...}
    //  int rhsAuto = getrhsAuto();
    //  nodes = _tt->_in->getNodeSetForKeyInStack(rhsAuto);

    // TR::Node *node = _tt->getNode();
    // TR::Node *firstChildNode = node->getFirstChild(); // since nullcheck would be its parent
    // if (firstChildNode)
    // {
    //     TR::Node *storeNode = firstChildNode;
    //     TR::Node *baseNode = storeNode->getFirstChild(); // base Node of store stmt
    //     TR::Node *storingValueNode = storeNode->getSecondChild(); //rhs node of store stmt

    //     if (storingValueNode->getOpCode().hasSymbolReference() && storingValueNode->getSymbolReference())
    //     {
    //         TR::SymbolReference *symRef = storingValueNode->getSymbolReference();
    //         TR::Symbol *sym = symRef->getSymbol();
    //         if (sym->getKind() == TR::Symbol::IsAutomatic)
    //         {
    //             int32_t slot = symRef->getCPIndex();
    //             std::cout << "slot for Store(rhs)is " << slot << "\n";
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
TR::SymbolReference *StoreStmt::getSymRef()
{
    TR::SymbolReference *symRef = nullptr;
    int lhsStackFieldLength = _stmtInfo->lhsFieldStack.size();
    symRef = _stmtInfo->lhsFieldStack[lhsStackFieldLength - 1];

    // works for basic a.f=b;
    //  TR::Node *node = _tt->getNode();
    //  TR::Node *firstChildNode = node->getFirstChild(); // since nullcheck would be its parent
    //  if (firstChildNode)                               // the awrtbari
    //  {
    //      symRef = firstChildNode->getSymbolReference();
    //      int32_t index = symRef->getCPIndex();
    //      std::cout << "(store)symRef " << symRef << "\n";
    //      std::cout << "index " << index << "\n";
    //  }

    return symRef;
}

int StoreStmt::getlhsAuto()
{

    return _stmtInfo->lhsAuto;
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
    //             std::cout << "slot for Store(lhs)is " << slot << "\n";
    //             return slot;
    //         }
    //     }
    // }
    // return -1;
}

int StoreStmt::getrhsAuto()
{
        return _stmtInfo->rhsAuto;

    // TR::Node *node = _tt->getNode();
    // TR::Node *firstChildNode = node->getFirstChild(); // since nullcheck would be its parent
    // if (firstChildNode)
    // {
    //     TR::Node *storeNode = firstChildNode;
    //     TR::Node *baseNode = storeNode->getFirstChild();          // base Node of store stmt
    //     TR::Node *storingValueNode = storeNode->getSecondChild(); // rhs node of store stmt

    //     if (storingValueNode->getOpCode().hasSymbolReference() && storingValueNode->getSymbolReference())
    //     {
    //         TR::SymbolReference *symRef = storingValueNode->getSymbolReference();
    //         TR::Symbol *sym = symRef->getSymbol();
    //         if (sym->getKind() == TR::Symbol::IsAutomatic)
    //         {
    //             int32_t slot = symRef->getCPIndex();
    //             std::cout << "slot for Store(rhs)is " << slot << "\n";
    //             return slot;
    //         }
    //     }
    // }
    // return -1;
}
