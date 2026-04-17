#ifndef WALK_OVER_TREE_IL_HPP
#define WALK_OVER_TREE_IL_HPP
namespace TR
{
  class Compilation;
  class TreeTop;
}
class WalkOverTreeIL
{
public:
  WalkOverTreeIL(TR::Compilation *comp);
  TR::Compilation *_comp;
  void walkTheTreeForInfo();
  void traverseBlock(TR::TreeTop *tt, TR::Compilation *comp);
};

#endif
