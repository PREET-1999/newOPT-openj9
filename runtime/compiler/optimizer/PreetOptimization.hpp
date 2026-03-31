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

#ifndef PREETOPTIMIZATION_INCL
#define PREETOPTIMIZATION_INCL

#include <stdint.h>
#include "codegen/CodeGenerator.hpp"
#include "compile/Compilation.hpp"
#include "il/DataTypes.hpp"
#include "il/ILOps.hpp"
#include "il/Node.hpp"
#include "il/Node_inlines.hpp"
#include "il/Symbol.hpp"
#include "optimizer/Optimization.hpp"
#include "optimizer/Optimization_inlines.hpp"
#include "optimizer/OptimizationManager.hpp"

namespace TR { class NodeChecklist; }

enum NodeType {
    NEW,
    STORE,
    YETTODECIDE

};
class TR_PreetOptimization : public TR::Optimization {
public:
   
    static TR::Optimization *create(TR::OptimizationManager *manager)
    {
        return new (manager->allocator()) TR_PreetOptimization(manager);
    }

    /** \brief
     *     Initializes the LoadExtensions codegen phase.
     *
     *  \param manager
     *     The optimization manager for this local optimization.
     */
    TR_PreetOptimization(TR::OptimizationManager *manager);

    /** \brief
     *     Performs the optimization on this compilation unit.
     *
     *  \return
     *     1 if any transformation was performed; 0 otherwise.
     */
    int32_t perform();
    void printTreeTop(TR::TreeTop *tt);
    void printNode(TR::Node *node);
    bool checkIfNewStmt(TR::TreeTop *tt);
    bool checkIfStoreStmt(TR::TreeTop *tt);
    bool checkIfCallStmt(TR::TreeTop *tt);

    NodeType findTreeTopType(TR::TreeTop* tt, TR::NodeChecklist  &visited);

    virtual const char *optDetailString() const throw() { return "O^O PREET OPTIMIZATION: "; }

    static int countOptimizationInvoked ;

};

#endif 