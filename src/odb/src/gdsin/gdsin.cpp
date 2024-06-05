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

  std::vector<int16_t> lastMod;
  for(int i = 0; i < 6; i++){
    lib->get_lastModified().push_back(r.data16[i]);
  }

  std::vector<int16_t> lastAccessed;
  for(int i = 6; i < 12; i++){
    lib->get_lastAccessed().push_back(r.data16[i]);
  }

  readRecord();
  checkRType(RecordType::LIBNAME);
  lib->setName(r.data8);

  readRecord();
  checkRType(RecordType::UNITS);

  lib->setUnits({r.data64[0], r.data64[1]});

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
  
  if(lib->getStructures().find(name) != lib->getStructures().end()){
        throw std::runtime_error("Corrupted GDS, Duplicate structure name");
  }

  dbGDSStructure* str =(dbGDSStructure*)new _dbGDSStructure((_dbDatabase*)db);
  str->setStrname(name);

  while(readRecord()){
    if(r.type == RecordType::ENDSTR){
      lib->getStructures().insert({name, str});
      std::cout << str->to_string() << std::endl;
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
  
  dbGDSBoundary* bdy = (dbGDSBoundary*)new _dbGDSBoundary((_dbDatabase*)db);

  readRecord();
  checkRType(RecordType::LAYER);
  bdy->setLayer(r.data16[0]);

  readRecord();
  checkRType(RecordType::DATATYPE);
  bdy->setDatatype(r.data16[0]);

  readRecord();
  checkRType(RecordType::XY);
  printf("r.data32.size(): %d\n", r.data32.size());
  for(int i = 0; i < r.data32.size(); i+=2){
    bdy->getXy().push_back({r.data32[i], r.data32[i + 1]});
  }

  readRecord();
  checkRType(RecordType::ENDEL);

  ((_dbGDSStructure*)&str)->_elements.push_back(bdy);
  return true;
}



}  // namespace odb
