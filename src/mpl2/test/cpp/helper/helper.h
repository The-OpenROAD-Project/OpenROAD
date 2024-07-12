#include "odb/db.h"

namespace mpl2 {

odb::dbDatabase* createSimpleDB();

odb::dbMaster* createSimpleMaster(odb::dbLib* lib,
                                  const char* name,
                                  uint width,
                                  uint height,
                                  const odb::dbMasterType& type,
                                  odb::dbTechLayer* layer);

odb::dbMPin* createMPinWithMTerm(odb::dbMaster* master,
                                 const char* mterm_name,
                                 const odb::dbIoType& io_type,
                                 const odb::dbSigType& sig_type,
                                 odb::dbTechLayer* layer,
                                 odb::Rect mpin_position);

odb::dbTrackGrid* createSimpleTrack(odb::dbBlock* block,
                                    odb::dbTechLayer* layer,
                                    int origin,
                                    int line_count,
                                    int step,
                                    int manufacturing_grid);

}  // namespace mpl2