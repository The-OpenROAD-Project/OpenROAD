#pragma once

#include <string>
#include <tuple>
#include <vector>

namespace utl {
class Logger;
}

namespace rmp {

// First string is type (mlatch/gate)
// Second string is master cell name
// Third vector is a list of all connections
typedef std::tuple<std::string, std::string, std::vector<std::string>> gate_;

class BlifParser
{
 public:
  BlifParser();
  bool parse(std::string& file_contents);
  void addInput(std::string& input);
  void addOutput(std::string& output);
  void addClock(std::string& clock);
  void addNewInstanceType(std::string& type);
  void addNewGate(std::string& cell_name);
  void addConnection(std::string& connection);
  void endParser();

  std::vector<std::string>& getInputs();
  std::vector<std::string>& getOutputs();
  std::vector<std::string>& getClocks();
  std::vector<gate_>& getGates();
  int getCombGateCount();
  int getFlopCount();

 private:
  std::vector<std::string> inputs;
  std::vector<std::string> outputs;
  std::vector<std::string> clocks;
  std::vector<gate_> gates;
  std::string currentGate, currentInstanceType;
  std::vector<std::string> currentConnections;
  int combCount, flopCount;
};

}  // namespace rmp