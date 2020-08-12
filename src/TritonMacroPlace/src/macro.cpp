#include "macro.h"
#include <iostream>

namespace MacroPlace{ 


Macro::Macro( std::string _name, std::string _type, 
        double _lx, double _ly, 
        double _w, double _h,
        double _haloX, double _haloY, 
        double _channelX, double _channelY,
        Vertex* _ptr, sta::Instance* _staInstPtr,
        odb::dbInst* _dbInstPtr) 
      : name(_name), type(_type), 
      lx(_lx), ly(_ly), 
      w(_w), h(_h),
      haloX(_haloX), haloY(_haloY),
      channelX(_channelX), channelY(_channelY), 
      ptr(_ptr), staInstPtr(_staInstPtr),
      dbInstPtr(_dbInstPtr) {}

void Macro::Dump() {
  std::cout << "MACRO " << name << " " 
    << type << " " 
    << lx << " " << ly << " " 
    << w << " " << h << std::endl;
  std::cout << haloX << " " << haloY << " " 
    << channelX << " " << channelY << std::endl;
}
}

MacroLocalInfo::MacroLocalInfo() : 
    haloX_(0), haloY_(0), 
    channelX_(0), channelY_(0) {}
