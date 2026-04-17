#include "optimizer/preetAnalysis/WalkOverTreeIL.hpp"
#include <iostream>
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
#include "WalkOverTreeIL.hpp"
#include "optimizer/preetAnalysis/IntraDataFlow.hpp"
#include "optimizer/preetAnalysis/statements/StatementKind.hpp"
#include "optimizer/preetAnalysis/statements/Statement.hpp"
#include "optimizer/preetAnalysis/statements/StoreStmt.hpp"
#include "optimizer/preetAnalysis/StatementInfoTable.hpp"
#include "optimizer/preetAnalysis/AuxillaryInfo.hpp"

bool isNodeSetLocal(std::vector<TR::Node *> nodeSet)
{

    for (auto node : nodeSet)
    {
        if (!AuxillaryInfo::isLocalAllocation(node))
            return false;
    }
    return true;
}

void addDebugCounters(TR::Compilation *comp, const char *debugCtrName, TR::TreeTop *treeTop)
{
    TR::DebugCounter::prependDebugCounter(comp, debugCtrName, treeTop);
}
WalkOverTreeIL::WalkOverTreeIL(TR::Compilation *comp)
{
    _comp = comp;
}

void WalkOverTreeIL::walkTheTreeForInfo()
{
    TR::ResolvedMethodSymbol *resolvedMethodSymbol = _comp->getMethodSymbol();
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

    if (verbose)
    {
        TR::CFG *cfg = _comp->getFlowGraph();

        // going over blocks
        for (TR::Block *block = _comp->getStartBlock(); block; block = block->getNextBlock())
        {
            if (!block->getEntry() || !block->getEntry()->getNode())
            {
                continue;
            }
            std::cout << "block " << block->getNumber();
            // process this block b
            if (block->getEntry())
                traverseBlock(block->getEntry(), _comp);
        }

        AuxillaryInfo::printTreeTopKinds(_comp);
    }
}

void WalkOverTreeIL::traverseBlock(TR::TreeTop *tt, TR::Compilation *comp)
{
    // just using this to get the findTreeTopLogic
    IntraDataFlow *cfgIdf = new IntraDataFlow(_comp);

    TR::Block *block = tt->getNode()->getBlock();
    for (TR::TreeTop *treeTop = block->getEntry();
         treeTop != block->getExit()->getNextTreeTop();
         treeTop = treeTop->getNextTreeTop())
    {
        TR::Node *node = treeTop->getNode();
        std::cout << "analysing node " << node << std::endl;

        std::pair<StatementKind, StatementInfoTable *> result = cfgIdf->findTreeTopType(treeTop);
        StatementKind sk = result.first;
        StatementInfoTable *stmtInfo = result.second;

        //just dump the kindOfTreeTOp encountered via the auxilary logger
        //this might need some check if done elseWhere, if already added in map...to avoid redundant adding: TODO
        AuxillaryInfo::insertTreeTopKind(sk,treeTop);

        switch (sk)
        {
        case StatementKind::FieldStoreStmt:
        {
            std::cout << "[" << node << "] is a store\n";
            StoreStmt *st = new StoreStmt(treeTop, stmtInfo);
            std::set<TR::Node *> baseNode = st->getNodePointedByBase();
            std::set<TR::Node *> storedNode = st->getNodeToBeStoredIntoBase();

            std::vector<TR::Node *> fromNodes;

            for (auto node : baseNode)
            {
                if (stmtInfo->lhsFieldStack.size() == 1)
                {
                    fromNodes.push_back(node);
                }
                else
                {
                    treeTop->_in->findNodes(node, stmtInfo->lhsFieldStack, 0, stmtInfo->lhsFieldStack.size() - 2, fromNodes);
                }
            }
            std::cout << "[";
            for (auto node : fromNodes)
            {
                std::cout << node << " ";
            }
            std::cout << "]\n";
            std::cout << " -->  [";

            std::vector<TR::Node *> toNodes;
            for (auto node : storedNode)
            {
                if (stmtInfo->rhsFieldStack.size() == 0)
                {
                    toNodes.push_back(node);
                }
                else
                {
                    treeTop->_in->findNodes(node, stmtInfo->rhsFieldStack, 0, stmtInfo->rhsFieldStack.size() - 1, toNodes);
                }
            }
            for (auto node : toNodes)
            {
                std::cout << node << " ";
            }
            std::cout << "]\n";

            bool isFromNodeSetLocal = isNodeSetLocal(fromNodes);
            bool isToNodeSetLocal = isNodeSetLocal(toNodes);
            std::cout << std::boolalpha // treats 1 and 0 as true and false for this particlar stream
                      << "isFromNodeSetLocal: " << isFromNodeSetLocal << "\n"
                      << "isToNodeSetLocal: " << isToNodeSetLocal << std::endl;

            // trying to add debugCounters
            const char *bothLocalCtr = TR::DebugCounter::debugCounterName(comp, "StoreInstance/fromToBothLocal");
            const char *eitherOneNonLocal = TR::DebugCounter::debugCounterName(comp, "StoreInstance/FromOrToNonLocal");

            if (isFromNodeSetLocal && isToNodeSetLocal)
                addDebugCounters(comp, bothLocalCtr, treeTop);
            else
                addDebugCounters(comp, eitherOneNonLocal, treeTop);
            AuxillaryInfo::getAuxillaryLogger()->printf("xyz\n");

            break;
        }
        }
    }
}
