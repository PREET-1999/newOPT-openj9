#ifndef INTRADATAFLOW_H
#define INTRADATAFLOW_H
// #include "optimizer/preetAnalysis/Treetop.hpp"
#include "il/TreeTop.hpp"
#include <map>
enum class StatementKind;
namespace TR
{
  class Compilation;
}
class StatementInfoTable;
class IntraDataFlow
{
public:
  IntraDataFlow(TR::Compilation* comp);
  TR::Compilation* _comp;
  std::pair<StatementKind, StatementInfoTable*>findTreeTopType(TR::TreeTop *tt);
  bool checkIfNewStmtViaStmtInfo(TR::TreeTop *tt,StatementInfoTable* stmtInfo);
  bool checkIfStoreStmtViaStmtInfo(TR::TreeTop *tt,StatementInfoTable* stmtInfo);
  bool checkIfLoadStmtViaStmtInfo(TR::TreeTop *tt,StatementInfoTable* stmtInfo);
  bool checkIfCopyStmtViaStmtInfo(TR::TreeTop *tt,StatementInfoTable* stmtInfo);
  bool checkIfCallStmtViaStmtInfo(TR::TreeTop *tt,StatementInfoTable* stmtInfo);


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

  //in process to byuild general findTreeTop
  std::map<int,int> globalNodeMap;
  void shoutOutLoud(TR::TreeTop* tt,StatementInfoTable* stmtInfo);
  void nodeDFS(TR::Node* node,StatementInfoTable* stmtInfo,bool forLhs);
};

#endif