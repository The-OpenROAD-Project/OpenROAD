#include <tcl.h>
#include "sta/StaMain.hh"
#include "openroad/OpenRoad.hh"
#include "tritonmp/MakeTritonMp.h"
#include "tritonmp/TritonMacroPlace.h"

namespace sta {
extern const char *tritonmp_tcl_inits[];
}

extern "C" {
extern int Mplace_Init(Tcl_Interp* interp);
}

namespace ord {

MacroPlace::TritonMacroPlace * 
makeTritonMp() 
{
  return new MacroPlace::TritonMacroPlace; 
}

void 
initTritonMp(OpenRoad *openroad) 
{
  Tcl_Interp* tcl_interp = openroad->tclInterp();
  Mplace_Init(tcl_interp);
  sta::evalTclInit(tcl_interp, sta::tritonmp_tcl_inits);
  openroad->getTritonMp()->setDb(openroad->getDb());
  openroad->getTritonMp()->setSta(openroad->getSta());
}

void
deleteTritonMp(MacroPlace::TritonMacroPlace *tritonmp)
{
  delete tritonmp;
}


}
