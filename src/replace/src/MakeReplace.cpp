#include <tcl.h>
#include "sta/StaMain.hh"
#include "openroad/OpenRoad.hh"
#include "replace/MakeReplace.h"
#include "replace/Replace.h"

namespace sta {
extern const char *replace_tcl_inits[];
}

extern "C" {
extern int Replace_Init(Tcl_Interp* interp);
}

namespace ord {

replace::Replace* 
makeReplace() {
  return new replace::Replace();
}

void
initReplace(OpenRoad* openroad) {
  Tcl_Interp* tcl_interp = openroad->tclInterp();
  Replace_Init(tcl_interp);
  sta::evalTclInit(tcl_interp, sta::replace_tcl_inits);
  openroad->getReplace()->setDb(openroad->getDb());
  openroad->getReplace()->setSta(openroad->getSta());
  openroad->getReplace()->setFastRoute(openroad->getFastRoute());
}

void
deleteReplace(replace::Replace *replace) {
  delete replace;
}

}
