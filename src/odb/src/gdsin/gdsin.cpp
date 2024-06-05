#include "gdsin.h"
#include "../db/dbGDSLib.h"
#include "../db/dbGDSStructure.h"
#include "../db/dbGDSBoundary.h"
#include "../db/dbGDSPath.h"
#include "../db/dbGDSSRef.h"
#include "../db/dbGDSElement.h"

namespace odb {

bool GDSReader::processLib(){
    
    readRecord();
    if(next_record.type != RecordType::BGNLIB)
      throw std::runtime_error("Corrupted GDS, missing BGNLIB !");
    
    lib = new _dbGDSLib();
    lib->_name = "GDS Library";

    readRecord();
    if(next_record.type != RecordType::LIBNAME)
      throw std::runtime_error("Corrupted GDS, missing LIBNAME !");
  }
}

}