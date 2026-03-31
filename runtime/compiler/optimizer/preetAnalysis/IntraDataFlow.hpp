#ifndef INTRADATAFLOW_H
#define INTRADATAFLOW_H
// #include "optimizer/preetAnalysis/Treetop.hpp"
#include "il/TreeTop.hpp"

enum class StatementKind;
namespace TR
{
  class Compilation;
}
class IntraDataFlow
{
public:
  StatementKind findTreeTopType(TR::TreeTop *tt);
  bool checkIfNewStmt(TR::TreeTop *tt);
  bool checkIfStoreStmt(TR::TreeTop *tt);
  bool checkIfLoadStmt(TR::TreeTop *tt);
  bool checkIfCopyStmt(TR::TreeTop *tt);
  bool checkIfCallStmt(TR::TreeTop *tt);

  void performAnalysis(TR::TreeTop *tt, TR::Compilation *comp);
  PTG *computeInSetFromPredecessor(PTG *pred);

  void setInSetOfFirstTreeTopOfBlock(TR::Block *block);
  void performAnalysisOverCFG(TR::Compilation *comp);

  PTG *mergePTG(PTG *one, PTG *another);
  PTG *mergePTGs(PTG **ptgs, int n);

  bool shouldPushSuccessorsBasedOnOutSets(int blockNumber, PTG *oldOut, PTG* newOut);
};

#endif