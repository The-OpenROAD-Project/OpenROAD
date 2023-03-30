#pragma once

namespace rsz {

class SteinerTree;

class AbstractSteinerRenderer
{
 public:
  virtual ~AbstractSteinerRenderer() = default;
  virtual void highlight(SteinerTree* tree) = 0;
};

}  // namespace rsz
