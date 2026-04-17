#ifndef AUXILLARYINFO_H
#define AUXILLARYINFO_H
#include<vector>
#include<map>
namespace TR{
    class Node;
    class TreeTop;
    class Compilation;
}
namespace OMR{
    class Logger;
}
enum class StatementKind;

class AuxillaryInfo{
    public:
    static std::map<TR::Node*,bool> localAllocations;
    static OMR::Logger* _auxillaryLogger;
    static std::map<StatementKind, std::vector<TR::TreeTop*>>treeTopKinds;

    static void setAuxillaryLogger( OMR::Logger* logger);
    static OMR::Logger* getAuxillaryLogger();
    static bool isLocalAllocation(TR::Node* node);

    static int insertTreeTopKind(StatementKind stmtKind, TR::TreeTop* tt);
static void printTreeTopKinds(TR::Compilation* comp);

};

#endif