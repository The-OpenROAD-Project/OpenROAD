#pragma once

namespace grt {

class AbstractRoutingCongestionDataSource
{
 public:
  virtual ~AbstractRoutingCongestionDataSource() = default;

  virtual bool canAdjustGrid() const = 0;
  virtual double getGridXSize() const = 0;
  virtual double getGridYSize() const = 0;

  virtual void registerHeatMap() = 0;
  virtual void update() = 0;
};

}  // namespace grt
