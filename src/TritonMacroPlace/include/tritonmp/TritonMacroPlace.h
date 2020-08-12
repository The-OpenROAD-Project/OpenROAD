#ifndef __MACRO_PLACER_EXTERNAL__
#define __MACRO_PLACER_EXTERNAL__

#include <memory>

namespace odb {
class dbDatabase;
}

namespace sta {
class dbSta;
}

namespace MacroPlace {

class MacroCircuit;

// TODO merge TritonMacroPlace and MacroCircuit class
class TritonMacroPlace {
public:
  TritonMacroPlace ();
  ~TritonMacroPlace ();

  void setDb(odb::dbDatabase* db);
  void setSta(sta::dbSta* sta);
  void setGlobalConfig(const char* globalConfig);
  void setLocalConfig(const char* localConfig);

  void setPlotEnable(bool mode);
  void setFenceRegion(double lx, double ly, double ux, double uy);

  bool placeMacros();
  int getSolutionCount();

private:
  std::unique_ptr<MacroCircuit> mckt_;
  int solCount_;
}; 

}

#endif
