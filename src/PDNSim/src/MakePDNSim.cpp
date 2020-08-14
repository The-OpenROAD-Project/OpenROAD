#include <tcl.h>
#include "sta/StaMain.hh"
#include "openroad/OpenRoad.hh"
#include "pdnsim/pdnsim.h"
#include "pdnsim/MakePDNSim.hh"

namespace sta {
extern const char *pdnsim_tcl_inits[];
}

extern "C" {
extern int Pdnsim_Init(Tcl_Interp* interp);
}

namespace ord {

pdnsim::PDNSim* makePDNSim() {
  return new pdnsim::PDNSim();
}

void
initPDNSim(OpenRoad* openroad) {
  Tcl_Interp* tcl_interp = openroad->tclInterp();
  Pdnsim_Init(tcl_interp);
  sta::evalTclInit(tcl_interp, sta::pdnsim_tcl_inits);
  openroad->getPDNSim()->setDb(openroad->getDb());
  openroad->getPDNSim()->setSta(openroad->getSta());
}

void
deletePDNSim(pdnsim::PDNSim *pdnsim) {
  delete pdnsim;
}

}
