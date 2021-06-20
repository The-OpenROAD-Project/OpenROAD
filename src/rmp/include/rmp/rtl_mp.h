#ifndef RTL_MP_H_
#define RTL_MP_H_

#include "rmp/shape_engine.h"
#include "rmp/block_placement.h"
#include "utl/Logger.h"

#include<iostream>
#include<string>
#include<vector>
#include<unordered_map>
#include<random>
#include<algorithm>

using utl::RMP;

namespace ord {
    void RTLMP(const char* config_file, utl::Logger*  logger);
}








#endif



