// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <vector>

#include "ResizerObserver.hh"
#include "odb/db.h"
#include "odb/geom.h"
#include "rsz/Resizer.hh"
#include "web/core.h"

namespace rsz {

class Graphics : public web::Renderer, public ResizerObserver
{
 public:
  Graphics();

  // ResizerObserver
  void setNet(odb::dbNet* net) override;
  void stopOnSubdivideStep(bool stop) override;
  void subdivideStart(odb::dbNet* net) override;
  void subdivide(const odb::Line& line) override;
  void subdivideDone() override;

  void repairNetStart(const BufferedNetPtr& bnet, odb::dbNet* net) override;
  void makeBuffer(odb::dbInst* inst) override;
  void repairNetDone() override;

  // Renderer
  void drawObjects(web::Painter& painter) override;

 private:
  void drawBNet(const BufferedNetPtr& bnet, web::Painter& painter);

  BufferedNetPtr bnet_;
  std::vector<odb::dbInst*> buffers_;
  odb::dbNet* net_{nullptr};
  std::vector<odb::Line> lines_;
  // Ignore this net if true
  bool subdivide_ignore_{false};
  bool repair_net_ignore_{false};
  // stop at each step?
  bool stop_on_subdivide_step_{false};
};

}  // namespace rsz
