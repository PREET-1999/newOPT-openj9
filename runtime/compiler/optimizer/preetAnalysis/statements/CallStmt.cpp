#include "CallStmt.hpp"
#include "il/Node.hpp"
#include "il/Node_inlines.hpp"
#include "il/SymbolReference.hpp"
#include "optimizer/preetAnalysis/statements/StatementKind.hpp"
#include "optimizer/preetAnalysis/statements/MethodCallKind.hpp"
#include <iostream>
#include <cstring>
#include <cstdio>
const char *toString(MethodCallKind kind)
{
    switch (kind)
    {
    case MethodCallKind::STATIC:
        return "STATIC";
    case MethodCallKind::VIRTUAL:
        return "VIRTUAL";
    case MethodCallKind::SPECIAL:
        return "SPECIAL";
    case MethodCallKind::INTERFACE:
        return "INTERFACE";
    case MethodCallKind::DYNAMIC:
        return "DYNAMIC";
    case MethodCallKind::NATIVE:
        return "NATIVE";
    case MethodCallKind::SOMEOTHERMETHODCALLKIND:
        return "SOMEOTHERMETHODCALLKIND";
    default:
        return "UNKNOWN";
    }
}

void printMethodCallKind(MethodCallKind kind)
{
    std::cout << toString(kind) << std::endl;
}
CallStmt::CallStmt(TR::TreeTop *tt,StatementInfoTable* stmtInfo)
{
    _tt = tt;
    _stmtInfo = stmtInfo;
}

PTG *CallStmt::Gen()
{
    std::cout << "[Gen CallStmt]\n";
    PTG *genPTG = new PTG();

    std::vector<TR::Node *> nodesPointedByArgs = inspectCallArguments();

    for (auto nodeFromArg : nodesPointedByArgs)
    {
        // instead of only the fields pointed by arg Obj currently present in heap, making obj -*-> as bottom
        // this was inintial version
        // std::vector<std::pair<TR::Node *, TR::SymbolReference *>> heapKeysForArgNode = _tt->_in->getHeapKeysWithNode(nodeFromArg);
        //  now keep inserting this heapKeys into genPTG heap
        //  actually only keys are needed, but for consistency also adding [key->set of nodes]
        //  for (auto key : heapKeysForArgNode)
        //  {
        //      genPTG->setPointsToOfKeyInHeapToBottom(key);
        //  }
        // initial version ends here

        //if nodeFromArg points to bottom, no need to add to heap
        if(nodeFromArg == nullptr)
            continue;



        TR::SymbolReference *starField = nullptr;
        TR::Node *bottom = nullptr;
        std::pair<TR::Node *, TR::SymbolReference *> NodeObjectField = {nodeFromArg, starField};
        genPTG->insertIntoHeap(NodeObjectField, bottom);
    }
    int lhsSlot = getAutoSlotIfNonVoidReturn(_tt->getNode());
    std::cout << "lhs slot for call " << lhsSlot << "\n";
    if (lhsSlot)
    {
        // there is a non void return
        genPTG->setPointsToOfKeyInStackToBottom(lhsSlot);
    }
       std::cout<<" [ CALL : GEN ]\n";
    genPTG->printStack();
    genPTG->printHeap();
    std::cout<<" [ ---CALL : GEN----- ]\n";
    return genPTG;
}

PTG *CallStmt::Kill()
{
    std::cout << "[CallStmt]\n";
    PTG *killPTG = new PTG();

    // some points to info would need to be killed based on args and method kinds
    // as of now just chcekng what all info can be fetched regarding the call
    MethodCallKind invokeKind = kindOfMethodCall();
    printMethodCallKind(invokeKind);

    // just for logging
    char *name = methodCallName();
    std::cout << name << std::endl;
    delete[] name;
    //--just for---logging

    std::vector<TR::Node *> nodesPointedByArgs = inspectCallArguments();

    for (auto nodeFromArg : nodesPointedByArgs)
    {
        std::vector<std::pair<TR::Node *, TR::SymbolReference *>> heapKeysForArgNode = _tt->_in->getHeapKeysWithNode(nodeFromArg);

        // now keep inserting this heapKeys into KillPTG heap
        // actually only keys are needed, but for consistency also adding [key->set of nodes]
        for (auto key : heapKeysForArgNode)
        {
            std::set<TR::Node *> nodeSet = _tt->_in->getNodeSetForKeyInHeap(key);
            for (auto node : nodeSet)
            {
                // actualy only key is required, but just adding to create PTG
                killPTG->insertIntoHeap(key, node);
            }
        }
    }
    int lhsSlot = getAutoSlotIfNonVoidReturn(_tt->getNode());
    if (lhsSlot)
    { // there is a non void return

        std::set<TR::Node *> nodeSet = _tt->_in->getNodeSetForKeyInStack(lhsSlot);
        for (auto node : nodeSet)
        {
            // actualy only key is required, but just adding to create PTG
            killPTG->insertIntoStack(lhsSlot, node);
        }
    }
       std::cout<<" [ CALL : KILL ]\n";
    killPTG->printStack();
    killPTG->printHeap();
    std::cout<<" [ ---CALL : KILL----- ]\n";
    return killPTG;
}

PTG *CallStmt::SetDiff(PTG *kill)
{
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

    std::map<int, std::set<TR::Node *>>::iterator sit;
    // std::set<TR::Node*>::iterator nodeIt;
    for (sit = kill->_stack.begin(); sit != kill->_stack.end(); ++sit)
    {
        std::map<int, std::set<TR::Node *>>::iterator skeyFoundIt;
        skeyFoundIt = copyIn->_stack.find(sit->first);
        if (skeyFoundIt != copyIn->_stack.end())
        {
            // cout<<"remove " <<it->first <<"?\n";
            copyIn->_stack.erase(skeyFoundIt);
        }
    }
    return copyIn;
}

PTG *CallStmt::SetUnion(PTG *filteredSet, PTG *newSet)
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

    std::map<int, std::set<TR::Node *>>::iterator sit;
    std::set<TR::Node *>::iterator nodeSIt;
    std::set<TR::Node *> snodes;
    int autoSlot;
    for (sit = newSet->_stack.begin(); sit != newSet->_stack.end(); ++sit)
    {
        autoSlot = sit->first;
        snodes = sit->second;
        for (auto node : snodes)
        {
            out->insertIntoStack(autoSlot, node);
        }
    }
    // out->printStack();
    // out->printHeap();
    return out;
}

MethodCallKind CallStmt::kindOfMethodCall()
{

    TR::Node *node = _tt->getNode();
    TR::Node *callNode;
    TR::SymbolReference *symRef;
    // since the call could be a child of some NULLCHeck or some other opcode....
    if (node->getOpCode().isCall())
    {
        callNode = node;
        symRef = callNode->getSymbolReference();
    }
    else if (node->getNumChildren() > 0 && node->getFirstChild()->getOpCode().isCall())
    {
        callNode = node->getFirstChild();
        symRef = callNode->getSymbolReference();
    }
    TR::Symbol *sym = symRef->getSymbol();
    if (sym)
    {
        switch (sym->getKind())
        {
        case TR::Symbol::IsResolvedMethod:
        case TR::Symbol::IsMethod:
        {
            TR::MethodSymbol *methodSym = sym->castToMethodSymbol();
            if (methodSym->isNative())
                return MethodCallKind::NATIVE;
            switch (methodSym->getMethodKind())
            {
            case TR::MethodSymbol::Virtual:
                return MethodCallKind::VIRTUAL;
            case TR::MethodSymbol::Interface:
                return MethodCallKind::INTERFACE;
            case TR::MethodSymbol::Static:
                return MethodCallKind::STATIC;
            case TR::MethodSymbol::Special:
                return MethodCallKind::SPECIAL;
            default:
                return MethodCallKind::SOMEOTHERMETHODCALLKIND;
            }
        }
        }
    }
    return MethodCallKind::SOMEOTHERMETHODCALLKIND;
}
static char *makeString(const char *str)
{
    size_t len = strlen(str) + 1;
    char *r = new char[len];
    strcpy(r, str);
    return r;
}
char *CallStmt::methodCallName()
{
    const char *fallback = "UnknownMethodName";

    TR::Node *node = _tt->getNode();
    TR::Node *callNode = nullptr;
    TR::SymbolReference *symRef = nullptr;

    if (node)
    {
        if (node->getOpCode().isCall())
            callNode = node;
        else if (node->getNumChildren() > 0 &&
                 node->getFirstChild()->getOpCode().isCall())
            callNode = node->getFirstChild();
    }

    if (callNode)
        symRef = callNode->getSymbolReference();

    if (symRef)
    {
        TR::Symbol *sym = symRef->getSymbol();
        if (sym &&
            (sym->getKind() == TR::Symbol::IsResolvedMethod ||
             sym->getKind() == TR::Symbol::IsMethod))
        {
            TR::Method *method =
                sym->castToMethodSymbol()->getMethod();

            if (method)
            {
                const char *cls = method->classNameChars();
                const char *name = method->nameChars();

                size_t len = strlen(cls) + strlen(name) + 2;
                char *result = new char[len];
                snprintf(result, len, "%s.%s", cls, name);
                return result;
            }
        }
    }

    return makeString(fallback);
}

// returns nodes pointed to by arguments
std::vector<TR::Node *> CallStmt::inspectCallArguments()
{
    std::vector<TR::Node *> nodesPointedByArgs{};
    TR::Node *node = _tt->getNode();
    TR::Node *callNode = nullptr;

    if (node)
    {
        if (node->getOpCode().isCall())
            callNode = node;
        else if (node->getNumChildren() > 0 &&
                 node->getFirstChild()->getOpCode().isCall())
            callNode = node->getFirstChild();
    }
    if (callNode)
    {
        int32_t firstArgIndex = callNode->getFirstArgumentIndex();

        // just printing the child nodes
        for (int32_t arg = firstArgIndex; arg < callNode->getNumChildren(); arg++)
        {
            TR::Node *argThChild = callNode->getChild(arg);
            std::cout << "child no : " << arg << " -> [" << argThChild << "]\n";
        }

        // assuming all args are of type 'a'load <auto slot>
        // get the argument slots
        for (int32_t arg = firstArgIndex; arg < callNode->getNumChildren(); arg++)
        {
            TR::Node *argThChild = callNode->getChild(arg);
            int slot = getArgumentAuto(argThChild);

            if (slot) // is slot 0 valid?, as of now assumed its invalid
            {
                std::cout << "arg " << arg << " -> { ";
                std::set<TR::Node *> nodeSetPointedToByArgthChild = _tt->_in->getNodeSetForKeyInStack(slot);
                for (auto node : nodeSetPointedToByArgthChild)
                {
                    std::cout << node << " ";
                    nodesPointedByArgs.push_back(node);
                }
                std::cout << "}\n";
            }
        }
    }
    return nodesPointedByArgs;
}

int CallStmt::getArgumentAuto(TR::Node *argNode)
{
    TR::Node *node = argNode;
    if (node->getOpCodeValue() == TR::aload)
    {
        if (node->getOpCode().hasSymbolReference() && node->getSymbolReference())
        {
            TR::SymbolReference *symRef = node->getSymbolReference();
            TR::Symbol *sym = symRef->getSymbol();
            if (sym->getKind() == TR::Symbol::IsAutomatic)
            {
                int32_t slot = symRef->getCPIndex();
                std::cout << "(arg) slot for [" << argNode << "] is " << slot << "\n";
                return slot;
            }
        }
    }
    return 0; // maybe this 0 can be used for "this"??if yes change this return value
}

int CallStmt::getAutoSlotIfNonVoidReturn(TR::Node *node)
{
    if (node->getOpCodeValue() == TR::astore)
    {
        if (node->getOpCode().hasSymbolReference() && node->getSymbolReference())
        {
            TR::SymbolReference *symRef = node->getSymbolReference();
            TR::Symbol *sym = symRef->getSymbol();
            if (sym->getKind() == TR::Symbol::IsAutomatic)
            {
                int32_t slot = symRef->getCPIndex();
                std::cout << "(lhs of call) slot for [" << node << "] is " << slot << "\n";
                return slot;
            }
        }
    }
    return 0; // maybe this 0 can be used for "this"??if yes change this return value
}
