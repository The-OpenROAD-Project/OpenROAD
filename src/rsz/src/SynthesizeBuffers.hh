#pragma once

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "sta/Sta.hh"
#include "sta/Search.hh"
#include "utl/Logger.h"

namespace rsz {

class Resizer;

using sta::dbNetwork;
using sta::dbSta;
using sta::Pin;
using sta::Net;
using sta::StaState;
using sta::LibertyCell;
using utl::Logger;
using sta::Vertex;
using sta::ArrivalVisitor;
using sta::RequiredVisitor;

class SynthesizeBuffers : StaState
{
 public:
  explicit SynthesizeBuffers(Resizer* resizer);
  ~SynthesizeBuffers() override;
  void synthesizeBuffers(int max_fanout, float gain, float slew);
  void init();

 private:
  bool getCin(Pin* drvr, float& cin);
  float requiredTime(Pin* load);
  bool collectLoads(Net *net, Pin *drvr_pin, std::vector<Pin *> &loads);
  float loadCapacitance(std::vector<Pin *> &loads);
  void synthesizeBuffers(Net* net, Pin* drvr_pin, int max_fanout, float gain);
  void clearSlewAnnotation(Vertex *vertex);

  std::vector<LibertyCell *> buffer_sizes;
  RequiredVisitor *req_visitor;
  ArrivalVisitor *arrival_visitor;

  Logger* logger_ = nullptr;
  dbSta* sta_ = nullptr;
  dbNetwork* db_network_ = nullptr;
  Resizer* resizer_;
};

}  // namespace rsz
