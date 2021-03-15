#ifndef MAKE_PDNSIM
#define MAKE_PDNSIM

namespace psm {
class PDNSim;
}

namespace ord {
class OpenRoad;

psm::PDNSim* makePDNSim();

void initPDNSim(OpenRoad* openroad);

void deletePDNSim(psm::PDNSim* pdnsim);

}  // namespace ord

#endif
