#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
#include <endian.h>

namespace odb {

class XML
{
 private:
  std::vector<XML> _children;
  std::string _name;
  std::string _value;

 public:
  XML();
  ~XML();

  void parseXML(std::string filename);
  std::string to_string(int depth) const;

  std::vector<XML>& getChildren();

  XML* findChild(std::string name);

  std::string getName();

  std::string getValue();
};


} // namespace odb