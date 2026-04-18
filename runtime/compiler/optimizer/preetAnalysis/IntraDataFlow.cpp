#include "optimizer/preetAnalysis/IntraDataFlow.hpp"
#include "optimizer/preetAnalysis/statements/StatementKind.hpp"
#include "optimizer/preetAnalysis/statements/Statement.hpp"
#include "optimizer/preetAnalysis/statements/ObjectAllocationStmt.hpp"
#include "optimizer/preetAnalysis/statements/StoreStmt.hpp"
#include "optimizer/preetAnalysis/statements/LoadStmt.hpp"
#include "optimizer/preetAnalysis/statements/CopyStmt.hpp"
#include "optimizer/preetAnalysis/statements/CallStmt.hpp"
#include "optimizer/preetAnalysis/statements/UnknownStmt.hpp"
#include "optimizer/preetAnalysis/AuxillaryInfo.hpp"


#include "codegen/CodeGenerator.hpp"
#include "compile/Compilation.hpp"
#include "control/Options.hpp"
#include "control/Options_inlines.hpp"
#include "env/CompilerEnv.hpp"
#include "env/StackMemoryRegion.hpp"
#include "env/TRMemory.hpp"
#include "il/DataTypes.hpp"
#include "il/ILOpCodes.hpp"
#include "il/ILOps.hpp"
#include "il/Node.hpp"
#include "il/Node_inlines.hpp"
#include "il/Symbol.hpp"
#include "il/TreeTop.hpp"
#include "il/TreeTop_inlines.hpp"
#include "infra/Assert.hpp"
#include "infra/BitVector.hpp"
#include "infra/Cfg.hpp"
#include "infra/SimpleRegex.hpp"
#include "infra/ILWalk.hpp"
#include "il/Node.hpp"

// #include "Operation.h"
// #include "Union.h"
// #include "Intersection.h"
// #include "StatementKind.h"
// #include "Statement.h"
// #include "ObjectAllocationStmt.h"
// #include "StoreStmt.h"
// #include "PTG.h"
#include <iostream>
#include "IntraDataFlow.hpp"
#include "StatementInfoTable.hpp"
using namespace std;

void IntraDataFlow::nodeDFS(TR::Node *node, StatementInfoTable *stmtInfo, bool forLhs)
{
    // std::cout << "processing node " << node << "\n";
    // if (globalNodeMap[node->getGlobalIndex()])
    // {
    //     std::cout << "Ive already processed node " << node << "\n";
    //     return;
    // }

    // // for (int32_t i = 0; i < node->getNumChildren(); i++)
    // // {
    // // This wrongly goes to explore chains even for unintended opcodes(eg calli)
    // TR::Node *child = node->getChild(0);
    // if (child)
    //     nodeDFS(child, stmtInfo, forLhs);
    // // }

    // the fix to only explore further the nodes whose opcode is of interest
    TR::Node *child = node->getChild(0);
    if (child && (child->getOpCodeValue() == TR::aload || child->getOpCodeValue() == TR::aloadi || child->getOpCodeValue() == TR::New))
    {
        std::cout << "from nodeDFS(" << node << ") calling nodeDFS(" << child << ")\n";
        nodeDFS(child, stmtInfo, forLhs);
    }
    else
    {
        if (child)
        {
            std::cout << "nodeDFS mein aage nodeDFS call nai huaa for node " << node << "\n";
        }
    }
    switch (node->getOpCodeValue())
    {
    case TR::aload:
    {
        TR::SymbolReference *symRef = node->getSymbolReference();
        TR::Symbol *sym = symRef->getSymbol();
        if (sym->getKind() == TR::Symbol::IsAutomatic)
        {
            int32_t slot = symRef->getCPIndex();
            std::cout << "(" << slot << ")";

            if (forLhs)
            {
                stmtInfo->setLHSAuto(slot);
            }
            else
            {
                stmtInfo->setRHSAuto(slot);
            }
        }
        break;
    }
    case TR::aloadi:
    {
        TR::SymbolReference *symRef = node->getSymbolReference();
        int32_t index = symRef->getCPIndex();
        std::cout << "." << index << "";

        if (forLhs)
        {
            stmtInfo->pushLHSField(symRef); // probably I need a queue :)...
        }
        else
        {
            stmtInfo->pushRHSField(symRef);
        }

        break;
    }
    case TR::New:
    {
        std::cout << "new ()";
        if (forLhs)
        {
            stmtInfo->setLHSNewNode(node); // probably I need a queue :)...
        }
        else
        {
            stmtInfo->setRHSNewNode(node);
        }

        break;
    }
    }

    // std::cout << "stored " << node << "\n";
    // globalNodeMap[node->getGlobalIndex()] = 1;
}
// in process of building generic findTreeTop
void IntraDataFlow::shoutOutLoud(TR::TreeTop *tt, StatementInfoTable *stmtInfo)
{
    TR::Node *node = tt->getNode();
    // only processing forms a = [] | [].f=[]
    switch (node->getOpCodeValue())
    {
    case TR::astore:
    {
        // for (int32_t i = 0; i < node->getNumChildren(); i++)
        // {
        TR::Node *child = node->getChild(0);
        if (child && (child->getOpCodeValue() == TR::aload || child->getOpCodeValue() == TR::aloadi || child->getOpCodeValue() == TR::New))
        {
            std::cout << "\n---------------------------------------\n";
            TR::SymbolReference *symRef = node->getSymbolReference();
            TR::Symbol *sym = symRef->getSymbol();
            if (sym->getKind() == TR::Symbol::IsAutomatic)
            {
                int32_t slot = symRef->getCPIndex();
                std::cout << "(" << slot << ") = ";
                stmtInfo->setLHSAuto(slot);
            }
            nodeDFS(child, stmtInfo, false);

            stmtInfo->printStatementInfo();
        }

        // }
        break;
    };
    case TR::ResolveCHK: //for handling 'this'
    case TR::ResolveAndNULLCHK:
    {
        TR::Node *firstChildNode = node->getFirstChild(); // since nullcheck would be its parent
        if (firstChildNode)
        {
            if (firstChildNode->getOpCode().isStoreIndirect()) // awrtbari
            {
                // for (int32_t i = 0; i < firstChildNode->getNumChildren(); i++)
                // {
                std::cout << "\n---------------------------------------\n";
                TR::Node *lhsChild = firstChildNode->getChild(0);
                if (lhsChild)
                    nodeDFS(lhsChild, stmtInfo, true);

                // this awrbari's .f
                TR::SymbolReference *symRef = firstChildNode->getSymbolReference();
                int32_t index = symRef->getCPIndex();
                std::cout << "." << index << "";
                stmtInfo->pushLHSField(symRef);

                std::cout << " = ";
                TR::Node *rhsChild = firstChildNode->getChild(1);
                if (rhsChild)
                    nodeDFS(rhsChild, stmtInfo, false);
                // }

                stmtInfo->printStatementInfo();
            }
        }
        break;
    };
    default:
    {
    }
    }
}
IntraDataFlow::IntraDataFlow(TR::Compilation *comp)
{
    _comp = comp;
}
std::pair<StatementKind, StatementInfoTable *>
IntraDataFlow::findTreeTopType(TR::TreeTop *tt)
{
    StatementInfoTable *currentStmtInfo = new StatementInfoTable();

    // see if I can get the .f.f.f.f's working
    shoutOutLoud(tt, currentStmtInfo);

    // once I have the statmentInfo instance populated for a "relevant stmt"
    // lets filter statement types based on that
    if (checkIfNewStmtViaStmtInfo(tt, currentStmtInfo))
    {
        std::cout << tt->getNode() << " is AllocationStmt\n";
        AuxillaryInfo::localAllocations[tt->getNode()->getFirstChild()]=true;
        return {StatementKind::AllocationStmt, currentStmtInfo};
    }
    if (checkIfStoreStmtViaStmtInfo(tt, currentStmtInfo))
    {
        std::cout << tt->getNode() << " is FieldStoreStmt\n";
        return {StatementKind::FieldStoreStmt, currentStmtInfo};
    }
    if (checkIfLoadStmtViaStmtInfo(tt, currentStmtInfo))
    {
        std::cout << tt->getNode() << " is FieldLoadStmt\n";
        return {StatementKind::FieldLoadStmt, currentStmtInfo};
    }
    if (checkIfCopyStmtViaStmtInfo(tt, currentStmtInfo))
    {
        std::cout << tt->getNode() << " is CopyStmt\n";
        return {StatementKind::CopyStmt, currentStmtInfo};
    }
    if (checkIfCallStmtViaStmtInfo(tt, currentStmtInfo))
    {
        std::cout << tt->getNode() << " is CallStmt\n";
        return {StatementKind::CallStmt, currentStmtInfo};
    }

    // trivially checking for each kind
    // if (checkIfNewStmt(tt))
    //     return StatementKind::AllocationStmt;
    // if (checkIfStoreStmt(tt))
    //     return StatementKind::FieldStoreStmt;
    // if (checkIfLoadStmt(tt))
    //     return StatementKind::FieldLoadStmt;
    // if (checkIfCopyStmt(tt))
    //     return StatementKind::CopyStmt;
    if (checkIfCallStmt(tt))
        return {StatementKind::CallStmt, currentStmtInfo};

    // to do other statemnts....

    return {StatementKind::YetToDecideStmt, currentStmtInfo};
}

bool IntraDataFlow::checkIfNewStmtViaStmtInfo(TR::TreeTop *tt, StatementInfoTable *stmtInfo)
{
    return (stmtInfo->lhsAuto != -1) &&         // LHS is a variable
           (!stmtInfo->hasLHSFields()) &&       // no LHS field chain
           (stmtInfo->rhsNewNode != nullptr) && // RHS is a new
           (!stmtInfo->hasRHSFields());         // no RHS field chain
}

bool IntraDataFlow::checkIfStoreStmtViaStmtInfo(TR::TreeTop *tt, StatementInfoTable *stmtInfo)
{
    bool lhsValid = 
                    (stmtInfo->hasLHSFields());

    // bool rhsValid = (stmtInfo->rhsNewNode != nullptr);

    // return lhsValid && rhsValid;

        return lhsValid ;

}

bool IntraDataFlow::checkIfLoadStmtViaStmtInfo(TR::TreeTop *tt, StatementInfoTable *stmtInfo)
{ // for now not hanling a=(new Node()).f...do I want to handle such??
    bool lhsValid = 
                    (!stmtInfo->hasLHSFields());

    bool rhsValid = 
                    (stmtInfo->hasRHSFields());

    return lhsValid && rhsValid;
}

bool IntraDataFlow::checkIfCopyStmtViaStmtInfo(TR::TreeTop *tt, StatementInfoTable *stmtInfo)
{
    bool lhsValid = (stmtInfo->lhsAuto != -1) &&
                    (!stmtInfo->hasLHSFields()) &&
                    (stmtInfo->lhsNewNode == nullptr);

    bool rhsValid = 
                    (!stmtInfo->hasRHSFields()) &&
                    (stmtInfo->rhsNewNode == nullptr);

    return lhsValid && rhsValid;
}

bool IntraDataFlow::checkIfCallStmtViaStmtInfo(TR::TreeTop *tt, StatementInfoTable *stmtInfo)
{
    return false;
}

bool IntraDataFlow::checkIfNewStmt(TR::TreeTop *tt)
{
    // OMR::Logger *log = comp()->log();

    TR::Node *node = tt->getNode();

    if (node->getOpCodeValue() == TR::astore)
    {
        // logprintf(trace(), log, " Checking new stmt %p\n", node);

        TR::Node *firstChildNode = node->getFirstChild();
        if (firstChildNode)
        {
            if (firstChildNode->getOpCodeValue() == TR::New)
            {
                // logprintf(trace(), log, "%p --> %p\n", node, firstChildNode);
                return true;
            }
        }
    }
    return false;
}
bool IntraDataFlow::checkIfStoreStmt(TR::TreeTop *tt)
{

    // OMR::Logger *log = comp()->log();

    TR::Node *node = tt->getNode();
    TR::Node *firstChildNode = node->getFirstChild(); // since nullcheck would be its parent
    if (firstChildNode)
    {
        if (firstChildNode->getOpCode().isStoreIndirect())
        {
            TR::Node *storeNode = firstChildNode;
            TR::Node *baseNode = storeNode->getFirstChild();
            TR::Node *storingValueNode = storeNode->getSecondChild();

            return true;
        }
    }

    // if(node->getOpCodeValue() == TR::astore ){
    //     logprintf(trace(), log, " Checking new stmt %p\n", node);

    //     TR::Node *firstChildNode = node->getFirstChild();
    //     // if(firstChildNode){
    //     //     if(firstChildNode->getOpCodeValue() == TR::New){
    //     //         logprintf(trace(), log, "%p --> %p\n", node , firstChildNode);
    //     //         return true;
    //     //     }
    //     // }

    // }
    return false;
}
bool IntraDataFlow::checkIfLoadStmt(TR::TreeTop *tt)
{
    TR::Node *node = tt->getNode();
    // if(!(node->getOpCode().isResolveOrNullCheck())) //since compressedRefs child can also be aloadi
    // return false;

    // the compressedrefs didnt have the lhs, this astore treetop has
    if (node->getOpCodeValue() != TR::astore) // since compressedRefs child can also be aloadi
        return false;

    TR::Node *firstChildNode = node->getFirstChild(); // since nullcheck would be its parent
    if (firstChildNode)
    {
        if (firstChildNode->getOpCode().isLoadIndirect())
        {
            TR::Node *loadNode = firstChildNode;
            std::cout << "first Child Node for [Load] " << loadNode << "\n";
            TR::Node *baseNode = loadNode->getFirstChild();

            TR::SymbolReference *symRef = loadNode->getSymbolReference();
            int32_t index = symRef->getCPIndex();
            std::cout << "(load)symRef " << symRef << "\n";
            std::cout << "(load)sym index " << index << "\n";
            // TR::Node *storingValueNode = storeNode->getSecondChild();

            return true;
        }
    }
    return false;
}
bool IntraDataFlow::checkIfCopyStmt(TR::TreeTop *tt)
{
    // astore
    //   aload
    TR::Node *node = tt->getNode();
    if (node->getOpCodeValue() == TR::astore)
    {
        TR::Node *firstChildNode = node->getFirstChild();
        if (firstChildNode)
        {
            if (firstChildNode->getOpCodeValue() == TR::aload)
                return true;
        }
    }
    return false;
}
bool IntraDataFlow::checkIfCallStmt(TR::TreeTop *tt)
{
    TR::Node *node = tt->getNode();

    // since the call could be a child of some NULLCHeck or some other opcode....
    if (node->getOpCode().isCall() || (node->getNumChildren() > 0 && node->getFirstChild()->getOpCode().isCall()))
    {
        return true;
    }
    return false;
}
// void IntraDataFlow::performAnalysis(TR::TreeTop *tt, TR::Compilation *comp)
// {

//     cout << "inside intraDF tt->" << tt << "\n";
//     cout << "inside intraDF node" << tt->getNode() << "\n";

//     // just trying if there is a name or id
//     //  tt->getNode()->getName(self()->comp()->getDebug())

//     // // Operation* op = new Intersection();
//     // // op->confluence();
//     TR::TreeTop *treeTop = tt;
//     // // dummy
//     // StatementKind sk = findTreeTopType(tt);
//     // StatementKind sk = StatementKind::NEW; //can we have an API to find type of TT TODO

//     // initially there is no parent of root treetop
//     PTG *predTreeTop = nullptr;
//     int verbose = 0;
//     // just printing inset to see how transfer functions have behaved
//     // if (treeTop == reinterpret_cast<TR::TreeTop *>(0x7fffc62a4330))
//     // {
//     //     verbose = 1;
//     // }

//     TR::ResolvedMethodSymbol *resolvedMethodSymbol = comp->getMethodSymbol();

//     const char *name = resolvedMethodSymbol->getResolvedMethod()->nameChars();
//     std::cout << "[" << name << "]\n";
//     std::cout << "resolvedMethodSymbol->getResolvedMethod()->nameChars() " << resolvedMethodSymbol->getResolvedMethod()->nameChars() << "\n";
//     if (strncmp(resolvedMethodSymbol->getResolvedMethod()->nameChars(),
//                 "processNodes", 12) == 0)
//     {
//         verbose = 1;
//         // std::cout << "Matched processNodes\n";
//     }

//     if (verbose)
//         for (treeTop; treeTop != NULL; treeTop = treeTop->getNextTreeTop())
//         {

//             std::cout << "analysing node " << treeTop->getNode() << std::endl;
//             StatementKind sk = findTreeTopType(treeTop);

//             if (predTreeTop) // first treetop's in shouldnt be changed
//                 treeTop->_in = computeInSetFromPredecessor(predTreeTop);

//             // just printing inset to see how transfer functions have behaved
//             if (verbose)
//             {
//                 treeTop->printDataFlow();
//             }

//             switch (sk)
//             {
//             case StatementKind::AllocationStmt:
//             {
//                 Statement *st = new ObjectAllocationStmt(treeTop);
//                 PTG *gen = st->Gen();
//                 PTG *kill = st->Kill();

//                 // set difference
//                 PTG *filteredSet = st->SetDiff(kill);
//                 // set union
//                 PTG *out = st->SetUnion(filteredSet, gen);
//                 treeTop->_out = out;

//                 break;
//             };
//             case StatementKind::FieldStoreStmt:
//             {
//                 Statement *st = new StoreStmt(treeTop);
//                 PTG *gen = st->Gen();
//                 PTG *kill = st->Kill();

//                 // set difference
//                 PTG *filteredSet = st->SetDiff(kill);
//                 // set union
//                 PTG *out = st->SetUnion(filteredSet, gen);
//                 treeTop->_out = out;

//                 break;
//             };
//             case StatementKind::FieldLoadStmt:
//             {
//                 Statement *st = new LoadStmt(treeTop);
//                 PTG *gen = st->Gen();
//                 PTG *kill = st->Kill();

//                 // set difference
//                 PTG *filteredSet = st->SetDiff(kill);
//                 // set union
//                 PTG *out = st->SetUnion(filteredSet, gen);
//                 treeTop->_out = out;

//                 break;
//             }
//             case StatementKind::CopyStmt:
//             {
//                 Statement *st = new CopyStmt(treeTop);
//                 PTG *gen = st->Gen();
//                 PTG *kill = st->Kill();

//                 // set difference
//                 PTG *filteredSet = st->SetDiff(kill);
//                 // set union
//                 PTG *out = st->SetUnion(filteredSet, gen);
//                 treeTop->_out = out;

//                 break;
//             }
//             case StatementKind::YetToDecideStmt:
//             {
//                 // as of now just flow the in to the out for such unhandled cases
//                 Statement *st = new UnknownStmt(treeTop);
//                 PTG *gen = st->Gen();
//                 PTG *kill = st->Kill();

//                 // set difference
//                 PTG *filteredSet = st->SetDiff(kill);
//                 // set union
//                 PTG *out = st->SetUnion(filteredSet, gen);
//                 treeTop->_out = out;
//                 break;
//             }
//             }
//             //     treeTop->printDataFlow();
//             predTreeTop = treeTop->_out;
//         }
// }

// preet just remove this as this was just to check whether you couldconfine treetops witin basic blocks
void IntraDataFlow::performAnalysis(TR::TreeTop *tt, TR::Compilation *comp)
{

    cout << "inside intraDF tt->" << tt << "\n";
    cout << "inside intraDF node" << tt->getNode() << "\n";

    // just trying if there is a name or id
    //  tt->getNode()->getName(self()->comp()->getDebug())

    // // Operation* op = new Intersection();
    // // op->confluence();
    TR::TreeTop *treeTop = tt;
    // // dummy
    // StatementKind sk = findTreeTopType(tt);
    // StatementKind sk = StatementKind::NEW; //can we have an API to find type of TT TODO

    // initially there is no parent of root treetop
    PTG *predTreeTop = nullptr;
    int verbose = 0;
    // just printing inset to see how transfer functions have behaved
    // if (treeTop == reinterpret_cast<TR::TreeTop *>(0x7fffc62a4330))
    // {
    //     verbose = 1;
    // }

    TR::ResolvedMethodSymbol *resolvedMethodSymbol = comp->getMethodSymbol();

    const char *name = resolvedMethodSymbol->getResolvedMethod()->nameChars();
    std::cout << "[" << name << "]\n";
    std::cout << "resolvedMethodSymbol->getResolvedMethod()->nameChars() " << resolvedMethodSymbol->getResolvedMethod()->nameChars() << "\n";
    if (strncmp(resolvedMethodSymbol->getResolvedMethod()->nameChars(),
                "processNodes", 12) == 0)
    {
        verbose = 1;
        // std::cout << "Matched processNodes\n";
    }

    // if (verbose)
    //     for (treeTop; treeTop != NULL; treeTop = treeTop->getNextTreeTop())
    //     {

    // std::cout << "analysing node " << treeTop->getNode() << std::endl;
    // StatementKind sk = findTreeTopType(treeTop);

    // if (predTreeTop) // first treetop's in shouldnt be changed
    //     treeTop->_in = computeInSetFromPredecessor(predTreeTop);

    // // just printing inset to see how transfer functions have behaved
    // if (verbose)
    // {
    //     treeTop->printDataFlow();
    // }

    // switch (sk)
    // {
    // case StatementKind::AllocationStmt:
    // {
    //     Statement *st = new ObjectAllocationStmt(treeTop);
    //     PTG *gen = st->Gen();
    //     PTG *kill = st->Kill();

    //     // set difference
    //     PTG *filteredSet = st->SetDiff(kill);
    //     // set union
    //     PTG *out = st->SetUnion(filteredSet, gen);
    //     treeTop->_out = out;

    //     break;
    // };
    // case StatementKind::FieldStoreStmt:
    // {
    //     Statement *st = new StoreStmt(treeTop);
    //     PTG *gen = st->Gen();
    //     PTG *kill = st->Kill();

    //     // set difference
    //     PTG *filteredSet = st->SetDiff(kill);
    //     // set union
    //     PTG *out = st->SetUnion(filteredSet, gen);
    //     treeTop->_out = out;

    //     break;
    // };
    // case StatementKind::FieldLoadStmt:
    // {
    //     Statement *st = new LoadStmt(treeTop);
    //     PTG *gen = st->Gen();
    //     PTG *kill = st->Kill();

    //     // set difference
    //     PTG *filteredSet = st->SetDiff(kill);
    //     // set union
    //     PTG *out = st->SetUnion(filteredSet, gen);
    //     treeTop->_out = out;

    //     break;
    // }
    // case StatementKind::CopyStmt:
    // {
    //     Statement *st = new CopyStmt(treeTop);
    //     PTG *gen = st->Gen();
    //     PTG *kill = st->Kill();

    //     // set difference
    //     PTG *filteredSet = st->SetDiff(kill);
    //     // set union
    //     PTG *out = st->SetUnion(filteredSet, gen);
    //     treeTop->_out = out;

    //     break;
    // }
    // case StatementKind::YetToDecideStmt:
    // {
    //     // as of now just flow the in to the out for such unhandled cases
    //     Statement *st = new UnknownStmt(treeTop);
    //     PTG *gen = st->Gen();
    //     PTG *kill = st->Kill();

    //     // set difference
    //     PTG *filteredSet = st->SetDiff(kill);
    //     // set union
    //     PTG *out = st->SetUnion(filteredSet, gen);
    //     treeTop->_out = out;
    //     break;
    // }
    // }
    // //     treeTop->printDataFlow();
    // predTreeTop = treeTop->_out;
    // }

    TR::Block *block = tt->getNode()->getBlock();

    // dummy (uncomment if you want to test specific calls without testing all kind of stmts)
    // for (TR::TreeTop *treeTop = block->getEntry();
    //      treeTop != block->getExit()->getNextTreeTop();
    //      treeTop = treeTop->getNextTreeTop())
    // {
    //     TR::Node *node = treeTop->getNode();
    //     // std::cout << "analysing node " << node << std::endl;

    //     // std::cout << "analysing node " << treeTop->getNode() << std::endl;
    //     StatementKind sk = findTreeTopType(treeTop);
    // }

    // actual one, uncomment this after dumy test done
    for (TR::TreeTop *treeTop = block->getEntry();
         treeTop != block->getExit()->getNextTreeTop();
         treeTop = treeTop->getNextTreeTop())
    {
        TR::Node *node = treeTop->getNode();
        std::cout << "analysing node " << node << std::endl;

        // std::cout << "analysing node " << treeTop->getNode() << std::endl;

        std::pair<StatementKind, StatementInfoTable *> result = findTreeTopType(treeTop);
        StatementKind sk = result.first;
        StatementInfoTable *stmtInfo = result.second;



        if (predTreeTop) // first treetop's in shouldnt be changed
            treeTop->_in = computeInSetFromPredecessor(predTreeTop);

        // just printing inset to see how transfer functions have behaved
        if (verbose)
        {
            treeTop->printDataFlow();
        }

        switch (sk)
        {
        case StatementKind::AllocationStmt:
        {
            Statement *st = new ObjectAllocationStmt(treeTop, stmtInfo);
            PTG *gen = st->Gen();
            PTG *kill = st->Kill();

            // set difference
            PTG *filteredSet = st->SetDiff(kill);
            // set union
            PTG *out = st->SetUnion(filteredSet, gen);
            treeTop->_out = out;

            break;
        };
        case StatementKind::FieldStoreStmt:
        {
            Statement *st = new StoreStmt(treeTop, stmtInfo);
            PTG *gen = st->Gen();
            PTG *kill = st->Kill();

            // set difference
            PTG *filteredSet = st->SetDiff(kill);
            // set union
            PTG *out = st->SetUnion(filteredSet, gen);
            treeTop->_out = out;

            break;
        };
        case StatementKind::FieldLoadStmt:
        {
            Statement *st = new LoadStmt(treeTop, stmtInfo);
            PTG *gen = st->Gen();
            PTG *kill = st->Kill();

            // set difference
            PTG *filteredSet = st->SetDiff(kill);
            // set union
            PTG *out = st->SetUnion(filteredSet, gen);
            treeTop->_out = out;

            break;
        }
        case StatementKind::CopyStmt:
        {
            Statement *st = new CopyStmt(treeTop, stmtInfo);
            PTG *gen = st->Gen();
            PTG *kill = st->Kill();

            // set difference
            PTG *filteredSet = st->SetDiff(kill);
            // set union
            PTG *out = st->SetUnion(filteredSet, gen);
            treeTop->_out = out;

            break;
        }
        case StatementKind::CallStmt: // for call stmt, a combined transfer flow might reduce computations...do we do that??

        {
            Statement *st = new CallStmt(treeTop, stmtInfo);
            PTG *gen = st->Gen();
            PTG *kill = st->Kill();

            // set difference
            PTG *filteredSet = st->SetDiff(kill);
            // set union
            PTG *out = st->SetUnion(filteredSet, gen);

            treeTop->_out = out;

            break;
        }
        case StatementKind::YetToDecideStmt:
        {
            // as of now just flow the in to the out for such unhandled cases
            Statement *st = new UnknownStmt(treeTop, stmtInfo);
            PTG *gen = st->Gen();
            PTG *kill = st->Kill();

            // set difference
            PTG *filteredSet = st->SetDiff(kill);
            // set union
            PTG *out = st->SetUnion(filteredSet, gen);
            treeTop->_out = out;
            break;
        }
        }
        treeTop->printDataFlow();
        predTreeTop = treeTop->_out;
    }
}

// might need to chenge prototype once actual pred blocks can be fetched
// currently just returning _out from passed predecessor ptg
PTG *IntraDataFlow::computeInSetFromPredecessor(PTG *pred)
{
    PTG *InFromPredOut = new PTG();
    /*
    IF works correctly we could move this into the constructor of the PTG with anotherPTG*
    */
    InFromPredOut->_stack = pred->_stack;
    InFromPredOut->_heap = pred->_heap;

    // //checking if pred and InfromPred would be tightly tied? (We dont wan that)
    // std::cout<<"Pred pehle\n";
    // pred->printStack();

    // //inserting into InFromPred some dummy
    // Node* dummy = new Node(11);
    // InFromPredOut->insertIntoStack(69,dummy);

    // cout<<"Pred after \n";
    // pred->printStack();

    // cout<<"In after \n";
    // InFromPredOut->printStack();

    return InFromPredOut;
}
void IntraDataFlow::setInSetOfFirstTreeTopOfBlock(TR::Block *block)
{

    // if block has no treeTops, no use to compute (entry/exit blocks)
    if (block->getEntry() == NULL)
        return;

    // first check whats the treeTops node of this block's first TreeTop
    // std::cout << "We would be calculating InSet for this TreeTop(node) " << block->getEntry()->getNode() << "\n";

    // for now just verify whose TreeTOp out sets will be used to calculate this block's in
    TR::CFGEdgeList preds = block->getPredecessors();
    int noOfPreds = preds.size();
    PTG *ptgs[noOfPreds];

    int ctr = 0;
    if (!preds.empty())
    {
        // std::cout << " with TreeTop(nodes) ";
        for (auto e = preds.begin(); e != preds.end(); ++e)
        {
            TR::Block *predBlock = toBlock((*e)->getFrom());
            if (predBlock != NULL)
            {
                TR::TreeTop *tt = predBlock->getExit(); // last TreeTop of the pred Blocks
                if (tt != NULL)
                {
                    std::cout << predBlock->getExit()->getNode() << " ";
                    ptgs[ctr] = predBlock->getExit()->_out;
                    ctr++;
                }
            }
            // std::cout << "\n";
        }
    }
    PTG *merged = new PTG();
    if (ctr > 0)
    {
        merged = mergePTGs(ptgs, noOfPreds);
        block->getEntry()->_in = merged;
    }
}
void IntraDataFlow::performAnalysisOverCFG(TR::Compilation *comp)
{

    TR::ResolvedMethodSymbol *resolvedMethodSymbol = comp->getMethodSymbol();
    int verbose = 0;
    const char *name = resolvedMethodSymbol->getResolvedMethod()->nameChars();
    std::cout << "[" << name << "]\n";
    std::cout << "resolvedMethodSymbol->getResolvedMethod()->nameChars() " << resolvedMethodSymbol->getResolvedMethod()->nameChars() << "\n";
    if (strncmp(resolvedMethodSymbol->getResolvedMethod()->nameChars(),
                "processNodes", 12) == 0)
    {
        verbose = 1;
        // std::cout << "Matched processNodes\n";
    }
    // get the cfg
    TR::CFG *cfg = comp->getFlowGraph();

    // create a worklist of CFGNodes or TR::BLOck?? whichone would be better
    List<TR::Block> workList(comp->trMemory());

    // preet uncomment till "uncomment till partial working worklist"
    //  // get one CFGNOde( maybe a Block) and push to the list
    //  TR ::Block *block;
    //  if (cfg != NULL)
    //  {
    //      // TR::CFGNode *cfgNode = cfg->getFirstNode(); //why is thsi first node 8 and not the entry
    //      TR::CFGNode *cfgNode = cfg->getStart();
    //      TR::Block *startBlock = toBlock(cfgNode);
    //      // if(verbose)
    //      //     std::cout<<"getStart() block number "<<startBlock->getNumber() <<std::endl;
    //      block = toBlock(cfgNode);
    //      workList.add(block);
    //  }

    // if (verbose)
    //     // loop over worklist till its not empty
    //     while (!workList.isEmpty())
    //     {
    //         TR::Block *b = workList.popHead();
    //         std::cout << "block no : " << b->getNumber();

    //         if (b->getEntry())
    //             std::cout << " its first node is " << b->getEntry()->getNode() << std::endl;

    //         // push successors into worlist
    // TR::CFGEdgeList succ = b->getSuccessors();
    // if (!succ.empty())
    // {
    //     std::cout << " with successors ";
    //     for (auto e = succ.begin(); e != succ.end(); ++e)
    //     {
    //         TR::Block *succBlock = toBlock((*e)->getTo());
    //         if (succBlock != NULL)
    //         {
    //             std::cout << succBlock->getNumber() << " , ";

    //             if (!workList.find(succBlock))
    //                 workList.add(succBlock);
    //         }
    //         std::cout << "\n";
    //     }
    // }

    //         // InsetOfEachBlock Test
    //         setInSetOfFirstTreeTopOfBlock(b);

    //         if (b->getEntry())
    //             performAnalysis(b->getEntry(), comp);
    //     }
    //"uncomment till partial working worklist"

    if (verbose)
    {

        // push all blocks in the worklist initiallyt

        for (TR::Block *block = comp->getStartBlock(); block; block = block->getNextBlock())
        {
            if (!block->getEntry() || !block->getEntry()->getNode())
            {
                std::cout << "WONT be pushing block " << block->getNumber() << "\n";
                continue;
            }
            std::cout << "Pushing block " << block->getNumber() << "to worklist" << "\n";
            workList.add(block);
        }

        // dummy check to see if a block which is already there in worklist is able to be detected;
        for (TR::Block *block = comp->getStartBlock(); block; block = block->getNextBlock())
        {
            if (!block->getEntry() || !block->getEntry()->getNode())
            {
                continue;
            }
            std::cout << "block " << block->getNumber();
            if (!workList.find(block))
            {
                workList.add(block);
                std::cout << "==> added";
            }
            else
                std::cout << "==> already in worklist";
        }

        // loop over worklist till its not empty
        while (!workList.isEmpty())
        {
            TR::Block *b = workList.popHead();
            std::cout << "From worklist, popped block no : " << b->getNumber();

            if (b->getEntry())
                std::cout << " its first node is " << b->getEntry()->getNode() << std::endl;

            PTG *previousOut;
            PTG *changedOut;
            if (b->getExit())
            {
                previousOut = b->getExit()->_out;
            }
            setInSetOfFirstTreeTopOfBlock(b);

            // process this block b
            if (b->getEntry())
                performAnalysis(b->getEntry(), comp);

            if (b->getExit())
                changedOut = b->getExit()->_out;

            // check if its successors are to be pushed ( deliberately only push 2's successors for now)
            if (shouldPushSuccessorsBasedOnOutSets(b->getNumber(), previousOut, changedOut))
            {
                std::cout << " check says block " << b->getNumber() << " 's successors are to be added\n";

                TR::CFGEdgeList succ = b->getSuccessors();
                if (!succ.empty())
                {
                    std::cout << " with successors ";
                    for (auto e = succ.begin(); e != succ.end(); ++e)
                    {
                        TR::Block *succBlock = toBlock((*e)->getTo());
                        if (succBlock != NULL && succBlock->getEntry())
                        {
                            std::cout << succBlock->getNumber() << " : ";

                            if (!workList.find(succBlock))
                            {
                                workList.add(succBlock);
                                std::cout << "==> added";
                            }
                            else
                                std::cout << "==> but already in worklist";
                            std::cout << "\n";
                        }
                        std::cout << "\n";
                    }
                }
            }
        }
    }
}

PTG *IntraDataFlow::mergePTG(PTG *one, PTG *another)
{
    // letsw merge and return the merged
    PTG *merged = new PTG();
    // lets start with stack merging moving pointer across "one"
    // this works with set union but is a bit difficult to manage
    // for (auto stackOne : one->_stack)
    // {
    //     std::set<TR::Node *> collector;

    //     std::cout << "picking " << stackOne.first << " -> [";

    //     auto stackAnother = another->_stack;
    //     // key of one is present in another
    //     auto anoIt = stackAnother.find(stackOne.first);
    //     if (anoIt != stackAnother.end())
    //     {
    //         // std::cout<<e.first <<" found in another \n";

    //         std::set_union(stackOne.second.begin(), stackOne.second.end(),
    //                        anoIt->second.begin(), anoIt->second.end(),
    //                        std::inserter(collector, collector.begin()));

    //         for (auto node : collector)
    //             merged->insertIntoStack(stackOne.first, node);
    //     }
    //     else
    //     {
    //     }

    //     for (auto obj : stackOne.second)
    //     {
    //         std::cout << "O" << obj->_id << " ";
    //     }
    //     std::cout << "]\n";

    //     cout << "post merge..";
    //     merged->printStack();
    // }

    /*
    STACK
    */
    auto stackAnother = another->_stack;
    auto stackOne = one->_stack;

    // lets start with stack merging moving pointer across "one"
    for (auto stackOnePair : stackOne)
    {
        int autoSlot = stackOnePair.first;

        // check if this key is also there in the another Stack
        auto stackAnother = another->_stack;
        auto anotherIt = stackAnother.find(autoSlot);
        if (anotherIt != stackAnother.end())
        {
            // key is also in another stack
            // get the set<TR::Node*> of another set and first set
            std::set<TR::Node *> stackOneNodeSet = stackOnePair.second;
            std::set<TR::Node *> stackAnotherNodeSet = anotherIt->second;
            std::set<TR::Node *> mergedNodeSet = stackOneNodeSet;
            // mergedNodeSet.merge(stackAnotherNodeSet);
            mergedNodeSet.insert(stackAnotherNodeSet.begin(), stackAnotherNodeSet.end());

            for (auto node : mergedNodeSet)
            {
                merged->insertIntoStack(autoSlot, node);
            }
        }
        else
        {
            // just push it as no match found
            std::set<TR::Node *> stackOneNodeSet = stackOnePair.second;
            for (auto node : stackOneNodeSet)
            {
                merged->insertIntoStack(autoSlot, node);
            }
        }
    }

    auto stackMerged = merged->_stack;
    // lets continue with stack merging moving pointer across "Another"
    for (auto stackAnotherPair : stackAnother)
    {
        int autoSlot = stackAnotherPair.first;

        // if this is not present in merged stack, push it

        auto anotherIt = stackMerged.find(autoSlot);
        if (anotherIt == stackMerged.end())
        {
            std::set<TR::Node *> stackAnotherNodeSet = stackAnotherPair.second;

            for (auto node : stackAnotherNodeSet)
            {
                merged->insertIntoStack(autoSlot, node);
            }
        }
    }

    merged->printStack();

    /*
    HEAP
    */
    auto heapOne = one->_heap;
    auto heapAnother = another->_heap;

    // traverse through first heap
    for (auto heapOnePair : heapOne)
    {
        auto objFieldPairOne = heapOnePair.first;
        auto heapAnotherIt = heapAnother.find(objFieldPairOne);
        if (heapAnotherIt != heapAnother.end())
        {
            std::set<TR::Node *> heapOneNodeSet = heapOnePair.second;
            std::set<TR::Node *> heapAnotherNodeSet = heapAnotherIt->second;
            std::set<TR::Node *> mergedNodeSet = heapOneNodeSet;
            // mergedNodeSet.merge(heapAnotherNodeSet);
            mergedNodeSet.insert(heapAnotherNodeSet.begin(), heapAnotherNodeSet.end());

            for (auto node : mergedNodeSet)
            {
                merged->insertIntoHeap(objFieldPairOne, node);
            }
        }
        else
        {
            // just push it as no match found
            std::set<TR::Node *> heapOneNodeSet = heapOnePair.second;
            for (auto node : heapOneNodeSet)
            {
                merged->insertIntoHeap(objFieldPairOne, node);
            }
        }
    }

    auto heapMerged = merged->_heap;
    // lets continue with heap merging moving pointer across "Another"
    for (auto heapAnotherPair : heapAnother)
    {
        auto objFieldPairAnother = heapAnotherPair.first;

        // if this is not present in merged heap, push it
        auto anotherIt = heapMerged.find(objFieldPairAnother);
        if (anotherIt == heapMerged.end())
        {
            std::set<TR::Node *> heapAnotherNodeSet = heapAnotherPair.second;

            for (auto node : heapAnotherNodeSet)
            {
                merged->insertIntoHeap(objFieldPairAnother, node);
            }
        }
    }
    merged->printHeap();

    return merged;
}

PTG *IntraDataFlow::mergePTGs(PTG **ptgs, int n)
{
    PTG *merged = new PTG();
    for (int i = 0; i < n; i++)
    {
        merged = mergePTG(merged, ptgs[i]);
    }

    std::cout << "Merged PTG in mergePTGOFAllPreds\n";
    merged->printStack();
    merged->printHeap();
    return merged;
}

bool IntraDataFlow::shouldPushSuccessorsBasedOnOutSets(int blockNumber, PTG *oldOut, PTG *newOut)
{
    // as of now just do a dummy true
    std::cout << "[Succesor Push Check] for block " << blockNumber << "\n";
    std::cout << "Previous Out \n";
    oldOut->printStack();
    oldOut->printHeap();
    std::cout << "New Out \n";
    newOut->printStack();
    newOut->printHeap();

    bool shouldPush = !(oldOut->equals(newOut));
    if (shouldPush)
        std::cout << "lets PUSHHHhhhhh\n";
    std::cout << "____________________________________________________________________\n";

    return shouldPush;
}
