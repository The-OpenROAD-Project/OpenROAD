#pragma once

#include "DataType.h"
#include "stt/SteinerTreeBuilder.h"

namespace grt {

enum class TreeStructure
{
  steinerTreeByStt,
  steinerTreeByFastroute
};

class AbstractFastRouteRenderer
{
 public:
  virtual ~AbstractFastRouteRenderer() = default;

  virtual void highlight(const FrNet* net) = 0;
  virtual void setSteinerTree(const stt::Tree& stree) = 0;
  virtual void setStTreeValues(const StTree& stree) = 0;
  virtual void setIs3DVisualization(bool is3DVisualization) = 0;
  virtual void setTreeStructure(TreeStructure treeStructure) = 0;

  virtual void redrawAndPause() = 0;
};

}  // namespace grt
