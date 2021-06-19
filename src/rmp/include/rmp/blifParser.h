#pragma once

#include <string>
#include <tuple>
#include <vector>

namespace utl {
class Logger;
}

namespace rmp {

class BlifParser
{
 public:
  BlifParser()
  {
    combCount = 0;
    flopCount = 0;
    currentInstanceType = "";
    currentGate = "";
  }
  bool parse(std::string& file_contents);
  void addInput(std::string& input) { inputs.push_back(input); }
  void addOutput(std::string& output) { outputs.push_back(output); }
  void addClock(std::string& clock) { clocks.push_back(clock); }
  void addNewInstanceType(std::string& type)
  {
    if (currentInstanceType != "") {
      gates.push_back(std::make_tuple(
          currentInstanceType, currentGate, currentConnections));
    }
    currentInstanceType = type;
    if (currentInstanceType == "mlatch")
      flopCount++;
    else if (currentInstanceType == "gate")
      combCount++;
    currentConnections.clear();
  }
  void addNewGate(std::string& cell_name) { currentGate = cell_name; }
  void addConnection(std::string& connection)
  {
    currentConnections.push_back(connection);
  }
  void endParser()
  {
    if (currentInstanceType != "") {
      gates.push_back(std::make_tuple(
          currentInstanceType, currentGate, currentConnections));
    }
  }

  std::vector<std::string>& getInputs() { return inputs; }
  std::vector<std::string>& getOutputs() { return outputs; }
  std::vector<std::string>& getClocks() { return clocks; }
  std::vector<std::tuple<std::string, std::string, std::vector<std::string>>>&
  getGates()
  {
    return gates;
  }
  int getCombGateCount() { return combCount; }
  int getFlopCount() { return flopCount; }

 private:
  std::vector<std::string> inputs;
  std::vector<std::string> outputs;
  std::vector<std::string> clocks;
  std::vector<std::tuple<std::string, std::string, std::vector<std::string>>>
      gates;
  std::string currentGate, currentInstanceType;
  std::vector<std::string> currentConnections;
  int combCount, flopCount;
};

}  // namespace rmp