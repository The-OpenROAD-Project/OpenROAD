#pragma once

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "sta/Sta.hh"
#include "sta/Search.hh"
#include "utl/Logger.h"

namespace rsz {

class Resizer;
class BufferedNet;
using BufferedNetPtr = std::shared_ptr<BufferedNet>;

using sta::dbNetwork;
using sta::dbSta;
using sta::Pin;
using sta::Net;
using sta::StaState;
using sta::LibertyCell;
using sta::Corner;
using utl::Logger;
using sta::Vertex;
using sta::ArrivalVisitor;
using sta::RequiredVisitor;
using odb::Point;

class TreeAmending : StaState
{
 public:
  explicit TreeAmending(Resizer* resizer);
  ~TreeAmending() override;
  void init();

  BufferedNetPtr amendedTree(const Pin *drvr_pin, const Corner *corner);

 private:
  BufferedNetPtr build(BufferedNet *root, float cutoff, float eps, float min_cap);
  BufferedNetPtr refineJunctionPlacement(BufferedNetPtr net);
  float estimatedWireCap(BufferedNet *p1, BufferedNet *p2);

  Logger* logger_ = nullptr;
  dbSta* sta_ = nullptr;
  dbNetwork* db_network_ = nullptr;
  Resizer* resizer_;
  const Corner* corner_ = nullptr;
};

}  // namespace rsz
