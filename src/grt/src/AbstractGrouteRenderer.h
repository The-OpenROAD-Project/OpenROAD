#pragma once

#include "odb/db.h"

namespace grt {

class AbstractGrouteRenderer
{
 public:
  virtual ~AbstractGrouteRenderer() = default;

  virtual void highlightRoute(odb::dbNet* net, bool show_pin_locations) = 0;

  virtual void clearRoute() = 0;
};

}  // namespace grt
