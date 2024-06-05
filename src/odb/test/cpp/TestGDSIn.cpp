#include <iostream>
#include <string>

// #include "odb/db.h"

#include "odb/gdsin.h"

int main(int argc, char* argv[])
{
  if(argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << " <gds file>" << std::endl;
    return 1;
  }
  odb::GDSReader reader(argv[1]);
  odb::dbGDSLib* lib = reader.read_gds();
}
