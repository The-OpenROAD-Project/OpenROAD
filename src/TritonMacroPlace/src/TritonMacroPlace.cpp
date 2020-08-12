#include "tritonmp/TritonMacroPlace.h"
#include "circuit.h"

namespace MacroPlace {

using namespace odb;

TritonMacroPlace::TritonMacroPlace() 
  : solCount_(0) { 
  std::unique_ptr<MacroCircuit> mckt(new MacroCircuit());
  mckt_ = std::move(mckt);
} 

TritonMacroPlace::~TritonMacroPlace() {}

void 
TritonMacroPlace::setDb(odb::dbDatabase* db) {
  mckt_->setDb(db);
}
void 
TritonMacroPlace::setSta(sta::dbSta* sta) {
  mckt_->setSta(sta);
}

void 
TritonMacroPlace::setGlobalConfig(const char* globalConfig) {
  mckt_->setGlobalConfig(globalConfig);
}

void 
TritonMacroPlace::setLocalConfig(const char* localConfig) {
  mckt_->setLocalConfig(localConfig);
}

void 
TritonMacroPlace::setPlotEnable(bool mode) {
  mckt_->setPlotEnable(mode);
}

void
TritonMacroPlace::setFenceRegion(double lx, double ly, double ux, double uy) {
  mckt_->setFenceRegion(lx, ly, ux, uy);
}


bool 
TritonMacroPlace::placeMacros() {
  mckt_->PlaceMacros(solCount_);
  return true;
}

int 
TritonMacroPlace::getSolutionCount() {
  return solCount_;
}

}
