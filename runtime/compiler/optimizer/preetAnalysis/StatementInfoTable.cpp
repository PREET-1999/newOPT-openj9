#include "StatementInfoTable.hpp"
#include "compile/Compilation.hpp"
#include "env/TRMemory.hpp"
#include <iostream>

StatementInfoTable::StatementInfoTable()
    : lhsAuto(-1),
      rhsAuto(-1),
      lhsNewNode(nullptr),
      rhsNewNode(nullptr)
{
}

// Reset everything for next statement
void StatementInfoTable::clear()
{
    lhsFieldStack.clear();
    rhsFieldStack.clear();

    lhsAuto = -1;
    rhsAuto = -1;
    lhsNewNode = nullptr;
    rhsNewNode = nullptr;
}

void StatementInfoTable::pushLHSField(TR::SymbolReference *symRef)
{
    lhsFieldStack.push_back(symRef);
}

void StatementInfoTable::pushRHSField(TR::SymbolReference *symRef)
{
    rhsFieldStack.push_back(symRef);
}

bool StatementInfoTable::hasLHSFields() const
{
    return !lhsFieldStack.empty();
}

bool StatementInfoTable::hasRHSFields() const
{
    return !rhsFieldStack.empty();
}

void StatementInfoTable::setLHSAuto(int value)
{
    lhsAuto = value;
}

void StatementInfoTable::setRHSAuto(int value)
{
    rhsAuto = value;
}

void StatementInfoTable::setLHSNewNode(TR::Node *node)
{
    lhsNewNode = node;
}

void StatementInfoTable::setRHSNewNode(TR::Node *node)
{
    rhsNewNode = node;
}

bool StatementInfoTable::isParam(int slot)
{
    // as of now param and this are considered same
    if (slot == -1)
        return true;
    return false;
}

bool StatementInfoTable::isThis(int slot)
{
    // as of now param and this are considered same
    if (slot == -1)
        return true;
    return false;
}

void StatementInfoTable::printStatementInfo()
{
    // Print lhsAuto
    std::cout << "LHS Auto: ";
    if (lhsAuto != -1)
        std::cout << lhsAuto;
    else
        std::cout << "empty";
    std::cout << std::endl;

    // Print rhsAuto
    std::cout << "RHS Auto: ";
    if (rhsAuto != -1)
        std::cout << rhsAuto;
    else
        std::cout << "empty";
    std::cout << std::endl;

    // Print vector sizes
    std::cout << "LHS Field Stack Size: "
              << lhsFieldStack.size() << std::endl;

    std::cout << "RHS Field Stack Size: "
              << rhsFieldStack.size() << std::endl;
}