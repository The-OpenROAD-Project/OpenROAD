
#include <iostream>
#include <string>

#include "odb/db.h"

#include "odb/gdsin.h"

using namespace odb;

int main(int argc, char* argv[])
{
  if(argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << " <gds file>" << std::endl;
    return 1;
  }
  GDSReader reader(argv[1]);
  dbGDSLib* lib = reader.read_gds();
  return 0;
}

