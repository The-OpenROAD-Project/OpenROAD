#pragma once

#include <string>
#include <vector>
#include<tuple>

namespace utl {
class Logger;
}

namespace rmp {

class BlifParser
{
 public:
  BlifParser() { combCount = 0; flopCount = 0; currentInstanceType=""; currentGate = ""; }
  bool parse(std::string s);
  void addInput(std::string s) { inputs.push_back(s); }
  void addOutput(std::string s) { outputs.push_back(s); }
  void addClock(std::string s) { clocks.push_back(s); }
  void addNewInstanceType(std::string s){
    if(currentInstanceType != ""){
      gates.push_back(std::make_tuple(currentInstanceType, currentGate, currentConnections));
    }
    currentInstanceType = s;
    if(currentInstanceType == "mlatch") flopCount++;
    else if(currentInstanceType == "gate") combCount++;
    currentConnections.clear();
  }
  void addNewGate(std::string s)
  {
    currentGate = s;
  }
  void addConnection(std::string s) { currentConnections.push_back(s); }
  void endParser()
  {
    if (currentInstanceType != "") {
      gates.push_back(std::make_tuple(currentInstanceType, currentGate, currentConnections));
    }
  }

  std::vector<std::string>& getInputs() { return inputs; }
  std::vector<std::string>& getOutputs() { return outputs; }
  std::vector<std::string>& getClocks() { return clocks; }
  std::vector<std::tuple<std::string, std::string, std::vector<std::string>>>& getGates() { return gates; }
  int getCombGateCount() {return combCount;}
  int getFlopCount() {return flopCount;}
  

 private:
  std::vector<std::string> inputs;
  std::vector<std::string> outputs;
  std::vector<std::string> clocks;
  std::vector<std::tuple<std::string, std::string, std::vector<std::string>>> gates;
  std::string currentGate, currentInstanceType;
  std::vector<std::string> currentConnections;
  int combCount, flopCount;
};

}  // namespace rmp