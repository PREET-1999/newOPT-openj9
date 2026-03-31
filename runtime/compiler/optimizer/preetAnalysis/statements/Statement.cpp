#include "Statement.hpp"
#include "il/PTG.hpp"
#include "il/Node.hpp"
#include "il/Node_inlines.hpp"
PTG *Statement::KillSetPostStrongUpdate(int searchKey, PTG *inSet)
{
        PTG* killPTG = new PTG();
        std::set<TR::Node*> nodes = inSet->getNodeSetForKeyInStack(searchKey);
        for(auto node : nodes){
            killPTG->insertIntoStack(searchKey, node);
        }
        return killPTG;
}