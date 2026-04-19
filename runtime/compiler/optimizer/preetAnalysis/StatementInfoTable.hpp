#ifndef STATEMENT_INFO_TABLE_HPP
#define STATEMENT_INFO_TABLE_HPP

#include <vector>
#include "il/PTG.hpp"
#include "il/Node.hpp"
#include "il/Node_inlines.hpp"
#include "il/SymbolReference.hpp"

// Forward declarations
namespace TR
{
class SymbolReference;
class Node;
class TR_Memory;
}

class StatementInfoTable
{
public:
    StatementInfoTable();

    // Reset state for processing next statement
    void clear();

    void pushLHSField(TR::SymbolReference* symRef);
    void pushRHSField(TR::SymbolReference* symRef);

    bool hasLHSFields() const;
    bool hasRHSFields() const;

    void setLHSAuto(int value);
    void setRHSAuto(int value);

    void setLHSNewNode(TR::Node* node);
    void setRHSNewNode(TR::Node* node);


    bool isParam(int slot);
    bool isThis(int slot);
    void printStatementInfo();

public:
    // Data members (kept public for simplicity)
    std::vector<TR::SymbolReference*> lhsFieldStack;
    std::vector<TR::SymbolReference*> rhsFieldStack;

    int lhsAuto;
    int rhsAuto;

    TR::Node* lhsNewNode;
    TR::Node* rhsNewNode;
};

#endif // STATEMENT_INFO_TABLE_HPP