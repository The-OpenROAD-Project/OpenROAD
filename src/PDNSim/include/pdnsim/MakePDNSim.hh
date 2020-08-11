#ifndef MAKE_PDNSIM
#define MAKE_PDNSIM

namespace pdnsim {
class PDNSim;
}

namespace ord {
class OpenRoad;

pdnsim::PDNSim* makePDNSim();

void
initPDNSim(OpenRoad* openroad);

void
deletePDNSim(pdnsim::PDNSim *pdnsim);

}

#endif
