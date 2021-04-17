#include <tcl.h>

#include "openroad/OpenRoad.hh"
#include "pdn/MakePdnGen.hh"
#include "sta/StaMain.hh"

namespace sta {

extern const char *pdn_tcl_inits[];

}

namespace pdn {
extern "C" {
extern int Pdn_Init(Tcl_Interp *interp);
}
}


namespace ord {

void
initPdnGen(OpenRoad *openroad)
{
  Tcl_Interp *interp = openroad->tclInterp();
  // Define swig TCL commands.
  pdn::Pdn_Init(interp);
  // Eval encoded sta TCL sources.
  sta::evalTclInit(interp, sta::pdn_tcl_inits);
}

} // namespace ord
