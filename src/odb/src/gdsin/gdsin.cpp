#include <iostream>

#include "gdsin.h"
#include "../db/dbGDSStructure.h"
#include "../db/dbGDSBoundary.h"
#include "../db/dbGDSLib.h"
#include "../db/dbGDSPath.h"
#include "../db/dbGDSElement.h"
#include "../db/dbGDSSRef.h"


namespace odb {

GDSReader::GDSReader(const std::string& filename) : lib(nullptr)
{
  db = dbDatabase::create();
  file.open(filename, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Could not open file");
  }
}

GDSReader::~GDSReader() 
{
  if (file.is_open()) {
    file.close();
  }
}

dbGDSLib* GDSReader::read_gds() 
{
  readRecord();
  checkRType(RecordType::HEADER);

  processLib();
  return lib;
}

bool GDSReader::checkRType(RecordType expect)
{
  if (r.type != expect) {
    std::string error_msg = "Corrupted GDS, Expected: " + std::string(recordNames[static_cast<int>(expect)])
     + " Got: " + std::string(recordNames[static_cast<int>(r.type)]);
    throw std::runtime_error(error_msg);
  }
  return true;
}

bool GDSReader::checkRData(DataType eType, size_t eSize)
{
  if (r.dataType != eType) {
    std::string error_msg = "Corrupted GDS, Expected data type: " + std::to_string(eType)
     + " Got: " + std::to_string(r.dataType);
    throw std::runtime_error("Corrupted GDS, Unexpected data type!");
  }
  return true;
}

double GDSReader::readReal8()
{
  uint64_t value;
  file.read(reinterpret_cast<char*>(&value), 8);
  return real8_to_double(htobe64(value));
}

int32_t GDSReader::readInt32() 
{
  int32_t value;
  file.read(reinterpret_cast<char*>(&value), 4);
  return htobe32(value);
}

int16_t GDSReader::readInt16() 
{
  int16_t value;
  file.read(reinterpret_cast<char*>(&value), 2);
  return htobe16(value);
}

int8_t GDSReader::readInt8()
{
  int8_t value;
  file.read(reinterpret_cast<char*>(&value), 1);
  return value;
}

bool GDSReader::readRecord()
{
  uint16_t recordLength = readInt16();
    uint8_t recordType = readInt8();
    uint8_t dataType = readInt8();

    r.type = toRecordType(recordType);
    r.dataType = toDataType(dataType);

    printf("Record Length: %d Record Type: %s Data Type: %d\n", recordLength, recordNames[recordType], dataType);

    if((recordLength-4) % dataTypeSize[r.dataType] != 0){
      throw std::runtime_error("Corrupted GDS, Data size is not a multiple of data type size!");
    }

    r.length = recordLength;
    int length = recordLength - 4;

    if(dataType == DataType::INT_2){
      r.data16.clear();
      for(int i = 0; i < length; i += 2){
        r.data16.push_back(readInt16());
      }
    }
    else if(dataType == DataType::INT_4 || dataType == DataType::REAL_4){
      r.data32.clear();
      for(int i = 0; i < length; i += 4){
        r.data32.push_back(readInt32());
      }
    }
    else if(dataType == DataType::REAL_8){
      r.data64.clear();
      for(int i = 0; i < length; i += 8){
        r.data64.push_back(readReal8());
      }
    }
    else if(dataType == DataType::ASCII_STRING || dataType == DataType::BIT_ARRAY){
      r.data8.clear();
      for(int i = 0; i < length; i++){
        r.data8.push_back(readInt8());
      }
    }
    
    if(file)
      return true;
    else
      return false;
}


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

  printf("UNITS: %ld %ld\n", r.data64[0], r.data64[1]);
  printf("UNITS HEX: %016lX %016lX\n", r.data64[0], r.data64[1]);
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
    case RecordType::SREF:
    case RecordType::AREF:
      processSRef(str);
      break;
    case RecordType::PATH:
      processPath(str);
      break;
    default:
      throw std::runtime_error("Unimplemented GDS Record Type");
      break;
  }
  return true;
}

bool GDSReader::processPath(dbGDSStructure& str){
  _dbGDSPath* path = new _dbGDSPath((_dbDatabase*)db);

  readRecord();
  checkRType(RecordType::LAYER);

  path->_layer = r.data16[0];

  readRecord();
  checkRType(RecordType::DATATYPE);

  path->_datatype = r.data16[0];

  readRecord();
  if(r.type == RecordType::PATHTYPE){
    path->_pathType = r.data16[0];
    readRecord();
  }
  else{
    path->_pathType = 0;
  }

  if(r.type == RecordType::WIDTH){
    path->_width = r.data32[0];
    readRecord();
  }
  else{
    path->_width = 0;
  }

  checkRType(RecordType::XY);

  for(int i = 0; i < r.data32.size(); i+=2){
    path->_xy.push_back({r.data32[i], r.data32[i + 1]});
  }

  readRecord();
  checkRType(RecordType::ENDEL);

  ((_dbGDSStructure*)&str)->_elements.push_back(path);
  return true;
}

bool GDSReader::processBoundary(dbGDSStructure& str){
  
  _dbGDSBoundary* bdy = new _dbGDSBoundary((_dbDatabase*)db);

  readRecord();
  checkRType(RecordType::LAYER);
  bdy->_layer = r.data16[0];

  readRecord();
  checkRType(RecordType::DATATYPE);
  bdy->_datatype = r.data16[0];

  readRecord();
  checkRType(RecordType::XY);

  for(int i = 0; i < r.data32.size(); i+=2){
    bdy->_xy.push_back({r.data32[i], r.data32[i + 1]});
  }

  readRecord();
  checkRType(RecordType::ENDEL);

  ((_dbGDSStructure*)&str)->_elements.push_back(bdy);
  return true;
}

bool GDSReader::processSRef(dbGDSStructure& str){

  _dbGDSSRef* sref = new _dbGDSSRef((_dbDatabase*)db);

  readRecord();
  checkRType(RecordType::SNAME);
  sref->_sName = std::string(r.data8.begin(), r.data8.end());

  readRecord();
  if(r.type == RecordType::STRANS){
    sref->_sTrans = processSTrans();
  }

  checkRType(RecordType::XY);
  for(int i = 0; i < r.data32.size(); i+=2){
    sref->_xy.push_back({r.data32[i], r.data32[i + 1]});
  }

  readRecord();
  if(r.type == RecordType::COLROW){
    sref->_colRow = {r.data16[0], r.data16[1]};
    readRecord();
  }
  else{
    sref->_colRow = {1, 1};
  }
  
  checkRType(RecordType::ENDEL);

  ((_dbGDSStructure*)&str)->_elements.push_back(sref);
  return true;
}

dbGDSSTrans GDSReader::processSTrans(){
  checkRType(RecordType::STRANS);
  printf("STRANS: %x\n", r.data8[0]);
  printf("STRANS: %x\n", r.data8[1]);

  bool flipX = r.data8[0] & 0x80;
  bool absMag = r.data8[1] & 0x04;
  bool absAngle = r.data8[1] & 0x02;

  readRecord();

  double mag = 1.0;
  if(r.type == RecordType::MAG){
    mag = r.data64[0];
    readRecord();
  }
  double angle = 0.0;
  if(r.type == RecordType::ANGLE){
    angle = r.data64[0];
    readRecord();
  }
  
  return dbGDSSTrans(flipX, absMag, absAngle, mag, angle);
}



}  // namespace odb
