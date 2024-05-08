#pragma once

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "sta/Sta.hh"
#include "utl/Logger.h"

namespace rsz {

class Resizer;

using sta::dbNetwork;
using sta::dbSta;
using sta::Pin;
using sta::Net;
using sta::StaState;
using utl::Logger;

class SynthesizeBuffers : StaState
{
 public:
  explicit SynthesizeBuffers(Resizer* resizer);
  ~SynthesizeBuffers() override;
  void synthesizeBuffers(int max_fanout, float gain, float slew);
  void init();

 private:
  bool getCin(Pin* drvr, float& cin);
  float requiredTime(const Pin* load);
  void synthesizeBuffers(Net* net, Pin* drvr_pin, int max_fanout, float gain, float slew);

  Logger* logger_ = nullptr;
  dbSta* sta_ = nullptr;
  dbNetwork* db_network_ = nullptr;
  Resizer* resizer_;
};

}  // namespace rsz
