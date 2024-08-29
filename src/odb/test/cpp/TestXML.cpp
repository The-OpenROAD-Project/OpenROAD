#include <iostream>
#include <map>
#include <string>

// #include "odb/db.h"

#include "odb/gdsUtil.h"

int main(int argc, char* argv[])
{
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <input file>" << std::endl;
    return 1;
  }

  std::map<std::pair<int16_t, int16_t>, std::string> map
      = odb::gds::getLayerMap(std::string(argv[1]));
  for (auto& pair : map) {
    std::cout << pair.first.first << " " << pair.first.second << " "
              << pair.second << std::endl;
  }
}
