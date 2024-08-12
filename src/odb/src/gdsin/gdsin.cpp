#include <iostream>

#include "gdsin.h"
#include "../db/dbGDSStructure.h"
#include "../db/dbGDSBoundary.h"
#include "../db/dbGDSLib.h"
#include "../db/dbGDSPath.h"
#include "../db/dbGDSElement.h"
#include "../db/dbGDSSRef.h"
#include "../db/dbGDSText.h"
#include "../db/dbGDSBox.h"

#define DEBUG 0

namespace odb {

GDSReader::GDSReader() : _lib(nullptr) { }

GDSReader::~GDSReader() 
{
  if (_file.is_open()) {
    _file.close();
  }
}

dbGDSLib* GDSReader::read_gds(const std::string& filename, dbDatabase* db) 
{
  _db = db;
  _file.open(filename, std::ios::binary);
  if (!_file) {
    throw std::runtime_error("Could not open file");
  }
  readRecord();
  checkRType(RecordType::HEADER);

  processLib();
  if (_file.is_open()) {
    _file.close();
  }
  _db = nullptr;
  return _lib;
}

bool GDSReader::checkRType(RecordType expect)
{
  if (_r.type != expect) {
    std::string error_msg = "Corrupted GDS, Expected: " + std::string(recordNames[static_cast<int>(expect)])
     + " Got: " + std::string(recordNames[static_cast<int>(_r.type)]);
    throw std::runtime_error(error_msg);
  }
  return true;
}

bool GDSReader::checkRData(DataType eType, size_t eSize)
{
  if (_r.dataType != eType) {
    std::string error_msg = "Corrupted GDS, Expected data type: " + std::to_string(eType)
     + " Got: " + std::to_string(_r.dataType);
    throw std::runtime_error("Corrupted GDS, Unexpected data type!");
  }
  return true;
}

double GDSReader::readReal8()
{
  uint64_t value;
  _file.read(reinterpret_cast<char*>(&value), 8);
  return real8_to_double(htobe64(value));
}

int32_t GDSReader::readInt32() 
{
  int32_t value;
  _file.read(reinterpret_cast<char*>(&value), 4);
  return htobe32(value);
}

int16_t GDSReader::readInt16() 
{
  int16_t value;
  _file.read(reinterpret_cast<char*>(&value), 2);
  return htobe16(value);
}

int8_t GDSReader::readInt8()
{
  int8_t value;
  _file.read(reinterpret_cast<char*>(&value), 1);
  return value;
}

bool GDSReader::readRecord()
{
  uint16_t recordLength = readInt16();
  uint8_t recordType = readInt8();
  uint8_t dataType = readInt8();
  _r.type = toRecordType(recordType);
  _r.dataType = toDataType(dataType);
  //printf("Record Length: %d Record Type: %s Data Type: %d\n", recordLength, recordNames[recordType], dataType);
  if((recordLength-4) % dataTypeSize[_r.dataType] != 0){
    throw std::runtime_error("Corrupted GDS, Data size is not a multiple of data type size!");
  }
  _r.length = recordLength;
  int length = recordLength - 4;
  if(dataType == DataType::INT_2){
    _r.data16.clear();
    for(int i = 0; i < length; i += 2){
      _r.data16.push_back(readInt16());
    }
  }
  else if(dataType == DataType::INT_4 || dataType == DataType::REAL_4){
    _r.data32.clear();
    for(int i = 0; i < length; i += 4){
      _r.data32.push_back(readInt32());
    }
  }
  else if(dataType == DataType::REAL_8){
    _r.data64.clear();
    for(int i = 0; i < length; i += 8){
      _r.data64.push_back(readReal8());
    }
  }
  else if(dataType == DataType::ASCII_STRING || dataType == DataType::BIT_ARRAY){
    _r.data8.clear();
    for(int i = 0; i < length; i++){
      _r.data8.push_back(readInt8());
    }
  }
  
  if(_file)
    return true;
  else
    return false;
}


bool GDSReader::processLib(){
  readRecord();
  checkRType(RecordType::BGNLIB);
    
  _lib = (dbGDSLib*)(new _dbGDSLib((_dbDatabase*)_db));

  if(_r.length != 28){
    throw std::runtime_error("Corrupted GDS, BGNLIB record length is not 28 bytes");
  }

  std::tm lastMT;
  lastMT.tm_year = _r.data16[0];
  lastMT.tm_mon = _r.data16[1];
  lastMT.tm_mday = _r.data16[2];
  lastMT.tm_hour = _r.data16[3];
  lastMT.tm_min = _r.data16[4];
  lastMT.tm_sec = _r.data16[5];

  _lib->set_lastModified(lastMT);

  std::tm lastAT;
  lastAT.tm_year = _r.data16[6];
  lastAT.tm_mon = _r.data16[7];
  lastAT.tm_mday = _r.data16[8];
  lastAT.tm_hour = _r.data16[9];
  lastAT.tm_min = _r.data16[10];
  lastAT.tm_sec = _r.data16[11];

  _lib->set_lastAccessed(lastAT);

  readRecord();
  checkRType(RecordType::LIBNAME);
  _lib->setLibname(_r.data8);

  readRecord();
  checkRType(RecordType::UNITS);

  //printf("UNITS: %f %f\n", _r.data64[0], _r.data64[1]);
  _lib->setUnits(_r.data64[0], _r.data64[1]);

  while(readRecord()){
    if(_r.type == RecordType::ENDLIB){
      return true;
    }
    else if(_r.type == RecordType::BGNSTR){
      if(!processStruct()){
        break;
      }
    }
  }

  delete _lib;
  _lib = nullptr;
  return false;
}

bool GDSReader::processStruct(){
  readRecord();
  checkRType(RecordType::STRNAME);
  
  std::string name = std::string(_r.data8.begin(), _r.data8.end());
  
  if(_lib->findGDSStructure(name.c_str()) != nullptr){
    throw std::runtime_error("Corrupted GDS, Duplicate structure name");
  }

  dbGDSStructure* str = dbGDSStructure::create(_lib, name.c_str());

  while(readRecord()){
    if(_r.type == RecordType::ENDSTR){
      if(DEBUG)
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

bool GDSReader::processXY(dbGDSElement* elem){
  checkRType(RecordType::XY);
  if(_r.data32.size() % 2 != 0){
    throw std::runtime_error("Corrupted GDS, XY data size is not a multiple of 2");
  }
  for(int i = 0; i < _r.data32.size(); i+=2){
    ((_dbGDSElement*)elem)->_xy.push_back({_r.data32[i], _r.data32[i + 1]});
  }
  return true;
}

void GDSReader::processPropAttr(dbGDSElement* elem){
  while(readRecord()){
    if(_r.type == RecordType::ENDEL){
      return;
    }
    
    checkRType(RecordType::PROPATTR);
    int16_t attr = _r.data16[0];
    
    readRecord();
    checkRType(RecordType::PROPVALUE);
    std::string value = _r.data8;
    
    elem->getPropattr().push_back({attr, value});
  }
}

bool GDSReader::processElement(dbGDSStructure& str){
  dbGDSElement* el = nullptr;

  switch(_r.type){
    case RecordType::BOUNDARY:
      el = processBoundary();
      break;
    case RecordType::SREF:
    case RecordType::AREF:
      el = processSRef();
      break;
    case RecordType::PATH:
      el = processPath();
      break;
    case RecordType::TEXT:
      el = processText();
      break;
    case RecordType::BOX:
      el = processBox();
      break;
    case RecordType::NODE:
      el = processNode();
      break;
    default:
      throw std::runtime_error("Unimplemented GDS Record Type");
      break;

  }

  processPropAttr(el);
  checkRType(RecordType::ENDEL);
  str.addElement(el);

  return true;
}

dbGDSElement* GDSReader::processPath(){
  _dbGDSPath* path = new _dbGDSPath((_dbDatabase*)_db);

  readRecord();
  checkRType(RecordType::LAYER);

  path->_layer = _r.data16[0];

  readRecord();
  checkRType(RecordType::DATATYPE);

  path->_datatype = _r.data16[0];

  readRecord();
  if(_r.type == RecordType::PATHTYPE){
    path->_pathType = _r.data16[0];
    readRecord();
  }
  else{
    path->_pathType = 0;
  }

  if(_r.type == RecordType::WIDTH){
    path->_width = _r.data32[0];
    readRecord();
  }
  else{
    path->_width = 0;
  }

  processXY((dbGDSElement*)path);

  return (dbGDSElement*)path;
}

dbGDSElement* GDSReader::processBoundary(){
  
  _dbGDSBoundary* bdy = new _dbGDSBoundary((_dbDatabase*)_db);

  readRecord();
  checkRType(RecordType::LAYER);
  bdy->_layer = _r.data16[0];

  readRecord();
  checkRType(RecordType::DATATYPE);
  bdy->_datatype = _r.data16[0];

  readRecord();
  processXY((dbGDSElement*)bdy);

  return (dbGDSElement*)bdy;
}

dbGDSElement* GDSReader::processSRef(){

  _dbGDSSRef* sref = new _dbGDSSRef((_dbDatabase*)_db);

  readRecord();
  checkRType(RecordType::SNAME);
  sref->_sName = std::string(_r.data8.begin(), _r.data8.end());

  readRecord();
  if(_r.type == RecordType::STRANS){
    sref->_sTrans = processSTrans();
  }

  processXY((dbGDSElement*)sref);

  readRecord();
  if(_r.type == RecordType::COLROW){
    sref->_colRow = {_r.data16[0], _r.data16[1]};
  }
  else{
    sref->_colRow = {1, 1};
  }

  return (dbGDSElement*)sref;
}

dbGDSElement* GDSReader::processText(){
  _dbGDSText* text = new _dbGDSText((_dbDatabase*)_db);

  readRecord();
  checkRType(RecordType::LAYER);
  text->_layer = _r.data16[0];

  readRecord();
  checkRType(RecordType::TEXTTYPE);
  text->_textType = _r.data16[0];

  readRecord();
  if(_r.type == RecordType::PRESENTATION){
    text->_presentation = processTextPres();
    readRecord();
  }

  if(_r.type == RecordType::PATHTYPE){
    text->_pathType = _r.data16[0];
    readRecord();
  }

  if(_r.type == RecordType::WIDTH){
    text->_width = _r.data32[0];
    readRecord();
  }

  if(_r.type == RecordType::STRANS){
    text->_sTrans = processSTrans();
  }

  processXY((dbGDSElement*)text);

  readRecord();
  checkRType(RecordType::STRING);
  text->_text = std::string(_r.data8.begin(), _r.data8.end());

  return (dbGDSElement*)text;
}

dbGDSElement* GDSReader::processBox(){
  _dbGDSBox* box = new _dbGDSBox((_dbDatabase*)_db);

  readRecord();
  checkRType(RecordType::LAYER);
  box->_layer = _r.data16[0];

  readRecord();
  checkRType(RecordType::BOXTYPE);
  box->_boxType = _r.data16[0];

  readRecord();
  processXY((dbGDSElement*)box);
  
  return (dbGDSElement*)(box);
}

dbGDSElement* GDSReader::processNode(){
  _dbGDSElement* elem = new _dbGDSElement((_dbDatabase*)_db);

  readRecord();
  checkRType(RecordType::LAYER);
  elem->_layer = _r.data16[0];

  readRecord();
  checkRType(RecordType::NODETYPE);
  elem->_datatype = _r.data16[0];

  processXY((dbGDSElement*)elem);

  return (dbGDSElement*)elem;
}

dbGDSSTrans GDSReader::processSTrans(){
  checkRType(RecordType::STRANS);

  bool flipX = _r.data8[0] & 0x80;
  bool absMag = _r.data8[1] & 0x04;
  bool absAngle = _r.data8[1] & 0x02;

  readRecord();

  double mag = 1.0;
  if(_r.type == RecordType::MAG){
    mag = _r.data64[0];
    readRecord();
  }
  double angle = 0.0;
  if(_r.type == RecordType::ANGLE){
    angle = _r.data64[0];
    readRecord();
  }
  
  return dbGDSSTrans(flipX, absMag, absAngle, mag, angle);
}

dbGDSTextPres GDSReader::processTextPres(){
  checkRType(RecordType::PRESENTATION);
  uint8_t hpres = _r.data8[1] & 0x3;
  uint8_t vpres = (_r.data8[1] & 0xC) >> 2;
  uint8_t font = (_r.data8[1] & 0x30) >> 4;

  //printf("\nFONT PRES DATA: %u \n", _r.data8[1]);

  return dbGDSTextPres(font, (dbGDSTextPres::VPres)vpres, (dbGDSTextPres::HPres)hpres);
}

}  // namespace odb
