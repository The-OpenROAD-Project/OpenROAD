
// Library reader routines
// Read in the liberty library using abc


#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <sta/Corner.hh>
#include <sta/PathRef.hh>
#include <sta/VertexVisitor.hh>

#include "base/abc/abc.h"
#include "base/main/abcapis.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "map/mio/mio.h"
#include "map/mio/mioInt.h"
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "rmp/FuncExpr2Kit.h"
#include "rmp/blif.h"
#include "sta/ConcreteNetwork.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/PathRef.hh"
#include "sta/PatternMatch.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/Sta.hh"
#include "utl/Logger.h"

#include "rmp/Restructure.h"
#include <base/io/ioAbc.h>
#include <base/main/main.h>
#include <base/main/mainInt.h>
#include <map/amap/amapInt.h>

using namespace abc;
namespace rmp {
	
LibRead::LibRead(utl::Logger* logger, char* lib_file_name, bool verbose)
    : logger_(logger), lib_file_name_(lib_file_name), verbose_(verbose)
{
  Abc_Start();
  std::string cmd("read_lib ");
  std::string cmd_line = cmd + lib_file_name_;
  Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
  Cmd_CommandExecute(pAbc, cmd_line.c_str());
  // Check we have a library
  if (Abc_FrameReadLibGen() == NULL)
    logger->report("Error. Library is not available\n");
  else
    logger->report("Successfully read in library\n");
}
}
