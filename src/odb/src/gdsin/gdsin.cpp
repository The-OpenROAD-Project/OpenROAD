#include <iostream>

#include "gdsin.h"
#include "../db/dbGDSStructure.h"
#include "../db/dbGDSBoundary.h"
#include "../db/dbGDSLib.h"
#include "../db/dbGDSPath.h"
#include "../db/dbGDSElement.h"
#include "../db/dbGDSSRef.h"


namespace odb {

bool GDSReader::processLib(){
  readRecord();
  checkRType(RecordType::BGNLIB);
    
  lib = (dbGDSLib*)(new _dbGDSLib((_dbDatabase*)db));

  if(r.length != 28){
    throw std::runtime_error("Corrupted GDS, BGNLIB record length is not 28 bytes");
  }

  std::tm lastMT;
  lastMT.tm_year = r.data16[0];
  lastMT.tm_mon = r.data16[1];
  lastMT.tm_mday = r.data16[2];
  lastMT.tm_hour = r.data16[3];
  lastMT.tm_min = r.data16[4];
  lastMT.tm_sec = r.data16[5];

  lib->set_lastModified(lastMT);

  std::tm lastAT;
  lastAT.tm_year = r.data16[6];
  lastAT.tm_mon = r.data16[7];
  lastAT.tm_mday = r.data16[8];
  lastAT.tm_hour = r.data16[9];
  lastAT.tm_min = r.data16[10];
  lastAT.tm_sec = r.data16[11];

  lib->set_lastAccessed(lastAT);

  readRecord();
  checkRType(RecordType::LIBNAME);
  lib->setLibname(r.data8);

  readRecord();
  checkRType(RecordType::UNITS);

  lib->setUnits(r.data64[0], r.data64[1]);

  while(readRecord()){
    if(r.type == RecordType::ENDLIB){
      return true;
    }
    else if(r.type == RecordType::BGNSTR){
      if(!processStruct()){
        break;
      }
    }
  }

  delete lib;
  lib = nullptr;
  return false;
}

bool GDSReader::processStruct(){
  readRecord();
  checkRType(RecordType::STRNAME);
  
  std::string name = std::string(r.data8.begin(), r.data8.end());
  
  if(lib->findGDSStructure(name.c_str()) != nullptr){
    throw std::runtime_error("Corrupted GDS, Duplicate structure name");
  }

  dbGDSStructure* str = dbGDSStructure::create(lib, name.c_str());

  while(readRecord()){
    if(r.type == RecordType::ENDSTR){
      std::cout << ((_dbGDSStructure*)str)->to_string() << std::endl;
      return true;
    }
    else{
      if(!processElement(*str)){
        break;
      }
    }
  }

  delete str;
  return false;
}

bool GDSReader::processElement(dbGDSStructure& str){
  switch(r.type){
    case RecordType::BOUNDARY:
      processBoundary(str);
      break;
    default:
      throw std::runtime_error("Unimplemented GDS Record Type");
      break;
  }
  return true;
}

bool GDSReader::processBoundary(dbGDSStructure& str){
  
  _dbGDSBoundary bdy((_dbDatabase*)db);

  readRecord();
  checkRType(RecordType::LAYER);
  bdy._layer = r.data16[0];

  readRecord();
  checkRType(RecordType::DATATYPE);
  bdy._datatype = r.data16[0];

  readRecord();
  checkRType(RecordType::XY);

  for(int i = 0; i < r.data32.size(); i+=2){
    bdy._xy.push_back({r.data32[i], r.data32[i + 1]});
  }

  readRecord();
  checkRType(RecordType::ENDEL);

  ((_dbGDSStructure*)&str)->_elements.push_back(bdy);
  return true;
}



}  // namespace odb
