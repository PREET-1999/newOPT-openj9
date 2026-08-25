#ifndef INTRADATAFLOW_H
#define INTRADATAFLOW_H
// #include "optimizer/preetAnalysis/Treetop.hpp"
#include "il/TreeTop.hpp"
#include <map>
#include<set>

enum class StatementKind;
namespace TR
{
  class Compilation;
}
class StatementInfoTable;
class IntraDataFlow
{
public:
//just a workAround for printing maymust, to add the second param
  // IntraDataFlow(TR::Compilation* comp); //original
    IntraDataFlow(TR::Compilation* comp, bool fixedPoint);

  TR::Compilation* _comp;
  std::pair<StatementKind, StatementInfoTable*>findTreeTopType(TR::TreeTop *tt);
  std::set<int> variablesToBeInitialized;
    std::set<int> paramsToBeInitialized;
    std::set<TR::SymbolReference*> fieldsToBeInitialized;

  bool isFixedPointAchieved;

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
  void populateLocalVariablesAndParams(TR::Compilation *comp);
  void populateFields(TR::Compilation *comp); //not sure if static will also be included


  void performAnalysisOverCFG(TR::Compilation *comp);

  PTG *mergePTG(PTG *one, PTG *another);
  PTG *mergePTGs(PTG **ptgs, int n);

  bool shouldPushSuccessorsBasedOnOutSets(int blockNumber, PTG *oldOut, PTG* newOut);

  //in process to byuild general findTreeTop
  std::map<int,std::set<TR::Node*>> globalNumberToNodeMap;
  std::set<TR::Node *>processNode(TR::Node *node,PTG *in,PTG* out);
  std::set<TR::Node *>processLoadNode(TR::Node *node,PTG *in,PTG *out);
  std::set<TR::Node *> processStoreNode(TR::Node *node,PTG *in,PTG* tempOut);
  std::set<TR::Node *> processCallNode(TR::Node *node,PTG *in,PTG* tempOut); //as of now just returning a node(for store to intercept as *) 
  std::set<TR::Node *> processNewNode(TR::Node *node,PTG *in,PTG* tempOut); //as of now just returning a node(for store to intercept as *) 

  void shoutOutLoud(TR::TreeTop* tt,StatementInfoTable* stmtInfo);
  void nodeDFS(TR::Node* node,StatementInfoTable* stmtInfo,bool forLhs);

  static std::set<std::string> failedMethods;

};

#endif