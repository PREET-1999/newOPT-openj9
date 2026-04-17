#include "AuxillaryInfo.hpp"
#include "optimizer/preetAnalysis/statements/StatementKind.hpp"
#include "il/TreeTop.hpp"
#include "il/TreeTop_inlines.hpp"
#include "compile/Compilation.hpp"
#include <vector>
#include <iostream>
std::map<TR::Node *, bool> AuxillaryInfo::localAllocations;
std::map<StatementKind, std::vector<TR::TreeTop *>> AuxillaryInfo::treeTopKinds;
OMR::Logger *AuxillaryInfo::_auxillaryLogger = nullptr;

void AuxillaryInfo::setAuxillaryLogger(OMR::Logger *logger)
{
    _auxillaryLogger = logger;
}

OMR::Logger *AuxillaryInfo::getAuxillaryLogger()
{
    return _auxillaryLogger;
}

bool AuxillaryInfo::isLocalAllocation(TR::Node *node)
{
    if (localAllocations[node])
        return true;

    return false;
}
const char *toString(StatementKind kind)
{
    switch (kind)
    {
    case StatementKind::NEW:
        return "NEW";
    case StatementKind::STORE:
        return "STORE";
    case StatementKind::AllocationStmt:
        return "AllocationStmt";
    case StatementKind::CopyStmt:
        return "CopyStmt";
    case StatementKind::FieldStoreStmt:
        return "FieldStoreStmt";
    case StatementKind::FieldLoadStmt:
        return "FieldLoadStmt";
    case StatementKind::CallStmt:
        return "CallStmt";
    case StatementKind::YetToDecideStmt:
        return "YetToDecideStmt";
    default:
        return "Unknown";
    }
}
int AuxillaryInfo::insertTreeTopKind(StatementKind stmtKind, TR::TreeTop *tt)
{
    std::cout << "inserting into TreeTopKind the tt(node) : " << tt->getNode() << "\n";
    std::cout << "statement kind: " << toString(stmtKind) << "\n";

    // this will create a new empty vector if no key present
    treeTopKinds[stmtKind].push_back(tt);

    return 0;
}

void AuxillaryInfo::printTreeTopKinds(TR::Compilation* comp)
{
    for (auto stmtKindTreeTopPair : treeTopKinds)
    {
        _auxillaryLogger->printf("StmtKind : %s\n",toString(stmtKindTreeTopPair.first));
        for(auto tt : stmtKindTreeTopPair.second){
            comp->getDebug()->print(_auxillaryLogger,tt);
        }
    }
}
