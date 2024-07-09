#include "odb/db.h"

namespace mpl2 {

odb::dbDatabase* createSimpleDB();

odb::dbMaster* createSimpleMaster(odb::dbLib* lib,
                                  const char* name,
                                  uint width,
                                  uint height,
                                  const odb::dbMasterType& type);

}  // namespace mpl2