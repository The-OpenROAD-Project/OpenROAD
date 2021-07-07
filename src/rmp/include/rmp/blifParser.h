#pragma once

#include <string>
#include <tuple>
#include <vector>

namespace utl {
class Logger;
}

namespace rmp {

enum class GateType
{
  None,
  Mlatch,
  Gate
};

struct Gate
{
  GateType type_;
  std::string master_;
  std::vector<std::string> connections_;
  Gate(const GateType& type,
       const std::string& master,
       const std::vector<std::string>& connections)
      : type_(type), master_(master), connections_(connections)
  {
  }
};

class BlifParser
{
 public:
  BlifParser();
  bool parse(std::string& file_contents);
  void addInput(const std::string& input);
  void addOutput(const std::string& output);
  void addClock(const std::string& clock);
  void addNewInstanceType(const std::string& type);
  void addNewGate(const std::string& cell_name);
  void addConnection(const std::string& connection);
  void endParser();

  const std::vector<std::string>& getInputs() const;
  const std::vector<std::string>& getOutputs() const;
  const std::vector<std::string>& getClocks() const;
  const std::vector<Gate>& getGates() const;
  int getCombGateCount() const;
  int getFlopCount() const;

 private:
  std::vector<std::string> inputs_;
  std::vector<std::string> outputs_;
  std::vector<std::string> clocks_;
  std::vector<Gate> gates_;
  std::string currentGate_;
  GateType currentInstanceType_;
  std::vector<std::string> currentConnections_;
  int combCount_;
  int flopCount_;
};

}  // namespace rmp
