// preet

/*******************************************************************************
 * Copyright IBM Corp. and others 2000
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution
 * and is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following Secondary
 * Licenses when the conditions for such availability set forth in the
 * Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
 * version 2 with the GNU Classpath Exception [1] and GNU General Public
 * License, version 2 with the OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] https://openjdk.org/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

#include "optimizer/PreetOptimization.hpp"

#include <stdint.h>
#include <string.h>
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
// #include "infra/Checklist.hpp"
#include "optimizer/Optimization.hpp"
#include "optimizer/Optimization_inlines.hpp"
#include "optimizer/Optimizer.hpp"
#include "optimizer/UseDefInfo.hpp"
#include "ras/Debug.hpp"
#include "ras/Logger.hpp"
#include "optimizer/preetAnalysis/IntraDataFlow.hpp"

// preet
#include <iostream>
#include "PreetOptimization.hpp"

int TR_PreetOptimization::countOptimizationInvoked = 0;
TR_PreetOptimization::TR_PreetOptimization(TR::OptimizationManager *manager)
    : TR::Optimization(manager)
{
}

void TR_PreetOptimization::printNode(TR::Node *node)
{

    std::cout << "Node is [" << node << "]\t";
    TR::ILOpCode &op = node->getOpCode();
    std::cout << "opcode's name is " << op.getName() << std::endl;

    int16_t numChildren = node->getNumChildren();
    std::cout << "[" << node << "] has " << numChildren << " childs" << std::endl;

    for (int i = 0; i < numChildren; i++)
    {
        printNode(node->getChild(i));
    }
}
NodeType TR_PreetOptimization ::findTreeTopType(TR ::TreeTop *tt, TR::NodeChecklist &visited)
{
    OMR::Logger *log = comp()->log();

    TR::Node *node = tt->getNode();
    logprintf(trace(), log, " looking at treetop %p -> Node \n", tt, node);

    TR::ILOpCode &op = node->getOpCode();

    int16_t numChildren = node->getNumChildren();

    // check if node is visited
    if (visited.contains(node))
    {
        logprintf(trace(), log, " TYPE : Already Processed %p\n", node);
        return NodeType ::YETTODECIDE; // FIXME should return already processed type
    }
    visited.add(node);

    // checking for "New"
    if (node->getOpCodeValue() == TR::New
        // ||
        // firstChildNode->getOpCodeValue() == TR::newvalue ||
        // firstChildNode->getOpCodeValue() == TR::newarray ||
        // firstChildNode->getOpCodeValue() == TR::anewarray
    )
    {
        logprintf(trace(), log, " TYPE : [New] node %p\n", node);
        return NodeType ::NEW;
    }
    // checking for "store" node
    if (node->getOpCode().isStore())
    {
        logprintf(trace(), log, " TYPE : [Store] node %p\n", node);
        return NodeType ::STORE;
    }

    if (numChildren)
    {
        TR::Node *firstChildNode = node->getFirstChild();
        visited.add(firstChildNode);

        if (visited.contains(firstChildNode))
        {
            logprintf(trace(), log, "  Already Processed  %p\n", node);
            return NodeType ::YETTODECIDE; // FIXME
        }

        // checking for "New"
        if (firstChildNode->getOpCodeValue() == TR::New
            // ||
            // firstChildNode->getOpCodeValue() == TR::newvalue ||
            // firstChildNode->getOpCodeValue() == TR::newarray ||
            // firstChildNode->getOpCodeValue() == TR::anewarray
        )
        {
            logprintf(trace(), log, " TYPE : [New] node %p\n", firstChildNode);
            return NodeType ::NEW;
        }

        // checking for "store" node
        if (firstChildNode->getOpCode().isStore())
        {
            logprintf(trace(), log, " TYPE : [Store] node %p\n", firstChildNode);
            return NodeType ::STORE;
        } // end store check
    }
    logprintf(trace(), log, " TYPE : Still not added support %p\n", node);
    return NodeType ::YETTODECIDE;
}
void TR_PreetOptimization::printTreeTop(TR::TreeTop *tt)
{
    std::cout << "\nTreetop is " << tt << std::endl;

    printNode(tt->getNode());
}

bool TR_PreetOptimization::checkIfNewStmt(TR::TreeTop *tt)
{
    OMR::Logger *log = comp()->log();

    TR::Node *node = tt->getNode();

    if (node->getOpCodeValue() == TR::astore)
    {
        logprintf(trace(), log, " Checking new stmt %p\n", node);

        TR::Node *firstChildNode = node->getFirstChild();
        if (firstChildNode)
        {
            if (firstChildNode->getOpCodeValue() == TR::New)
            {
                logprintf(trace(), log, "%p --> %p\n", node, firstChildNode);
                return true;
            }
        }
    }
    return false;
}

bool TR_PreetOptimization::checkIfStoreStmt(TR::TreeTop *tt)
{
    OMR::Logger *log = comp()->log();

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

bool TR_PreetOptimization::checkIfCallStmt(TR::TreeTop *tt)
{
    TR::Node *node = tt->getNode();

    //since the call could be a child of some NULLCHeck or some other opcode....
    if (node->getOpCode().isCall() || (node->getNumChildren() > 0 &&  node->getFirstChild()->getOpCode().isCall())){
        return true;
    }
    return false;
}

void printCFGNode(TR::CFGNode *cfgNode){
    TR::Block *block = (TR::Block*)cfgNode;
    std::cout<<"block No" <<block->getNumber() <<std::endl;
    std::cout<<"block name " <<block <<std::endl;
    if(!block->getEntry()){
        std::cout<<"Entry | Exit block\n" <<std::endl;
    }
    else{
        std::cout<<"Treetop->Node of this block starts at "<<block->getEntry()->getNode() <<std::endl;
    }

}

int32_t TR_PreetOptimization::perform()
{
    countOptimizationInvoked++;

    OMR::Logger *log = comp()->log();
    logprintf(trace(), log, "Performing Preet Optimization %d.\n", countOptimizationInvoked);

    if (trace())
    {
        comp()->dumpMethodTrees(log, "Trees before Preet Optimization");

        // traversing via loops
        TR::Compilation *compilation = comp();
        TR::ResolvedMethodSymbol *resolvedMethodSymbol = compilation->getMethodSymbol();

        TR::TreeTop *tt = resolvedMethodSymbol->getFirstTreeTop();
        TR::TreeTop *head = tt; //to keep the original tt stored
        // finding type of treetops like new load store
        // logprints(trace(), log, " Finding treeTop type \n");

        TR::NodeChecklist visited(comp());
        // for (; tt; tt = tt->getNextTreeTop())
        // {
        //     NodeType treeTopType = findTreeTopType(tt, visited);
        // }

        //initializing treetops in and out sets
        log->printf("Initializing In/Out sets of treeTops\n");
        log->preetPrintf(__FILE__,"Worked kya Initializing In/Out sets of treeTops\n");
        for (; tt; tt = tt->getNextTreeTop())
        {
                tt->initializeInAndOutSets();
        }
        // for (; tt; tt = tt->getNextTreeTop())
        // {
        //     bool isNew = checkIfNewStmt(tt);
        //     bool isStore = checkIfStoreStmt(tt);

        //     //----------------IntraDataFlowAnalysis checking for new as of now-----------------
        //     if (isNew || isStore)
        //     {
        //         tt->initializeInAndOutSets();

        //         IntraDataFlow *idf = new IntraDataFlow();
        //         idf->performAnalysis(tt);
        //     }

        //     // tt->initializeInAndOutSets();

        //     // IntraDataFlow *idf = new IntraDataFlow();
        //     // idf->performAnalysis(tt);
        // }
        
        //perform dataflow over the method starting from head tt
        // IntraDataFlow *idf = new IntraDataFlow();
        // idf->performAnalysis(head,comp());
        
        IntraDataFlow *cfgIdf = new IntraDataFlow();
        cfgIdf->performAnalysisOverCFG(comp());


        // for(;tt;tt=tt->getNextTreeTop()){
        //     printTreeTop(tt);
        // }

        // //finding what informations I can fetch from treetops
        // TR::Node* treeTopNode = tt->getNode();
        // std::cout<<"Treetop's node is " <<treeTopNode <<std::endl;
        // treeTopNode->printFullSubtree();

        // TR::ILOpCode &op = treeTopNode->getOpCode();
        // std::cout<<"opcode's name is " <<op.getName() <<std::endl;

        //----------------IntraDataFlowAnalysis-----------------
        // IL il;
        // //crearting linked list of TreeTops
        // Treetop* head = il.createTreeTops();
        // il.printTreeTops(head);

        // IntraDataFlow *idf = new IntraDataFlow();
        // idf->performAnalysis(head);
    }


//why do below stmts get printd even during "make all"
    // TR::Compilation *compilation = comp();
    // TR::ResolvedMethodSymbol *resolvedMethodSymbol = compilation->getMethodSymbol();
    // std::cout<<"\n------------------------------------------------------------------------\n";
    // std::cout<<resolvedMethodSymbol->getResolvedMethod()->nameChars()<<"\n";
    // std::cout<<resolvedMethodSymbol->getResolvedMethod()->signatureChars()<<"\n";

    // // cfg related
    // TR::CFG * cfg = comp()->getFlowGraph();
    // TR::CFGNode *node;
    // for (node = cfg->getFirstNode(); node; node = node->getNext()) {
    //     std::cout<<"node " <<node <<" has number " <<node->getNumber() <<std::endl;

    //     printCFGNode(node);
    // }
    return 0;
}
