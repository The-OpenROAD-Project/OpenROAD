#ifndef RTL_MP_H_
#define RTL_MP_H_

#include <algorithm>
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "rmp/block_placement.h"
#include "rmp/shape_engine.h"
#include "utl/Logger.h"

using utl::RMP;

namespace ord {
bool RTLMP(const char* config_file, utl::Logger* logger);
}

#endif
