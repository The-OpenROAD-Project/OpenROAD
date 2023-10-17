#include "ScanPin.hh"

namespace dft {

ScanPin::ScanPin(odb::dbITerm* iterm) : value_(iterm)
{
}

ScanPin::ScanPin(odb::dbBTerm* bterm) : value_(bterm)
{
}

odb::dbNet* ScanPin::getNet() const
{
  return std::visit(
      overloaded{[](odb::dbITerm* iterm) { return iterm->getNet(); },
                 [](odb::dbBTerm* iterm) { return iterm->getNet(); }},
      value_);
}

std::string_view ScanPin::getName() const
{
  return std::visit(
      overloaded{
          [](odb::dbITerm* iterm) { return iterm->getMTerm()->getConstName(); },
          [](odb::dbBTerm* iterm) { return iterm->getConstName(); }},
      value_);
}

const std::variant<odb::dbITerm*, odb::dbBTerm*>& ScanPin::getValue() const
{
  return value_;
}

ScanLoad::ScanLoad(odb::dbITerm* iterm) : ScanPin(iterm)
{
}

ScanLoad::ScanLoad(odb::dbBTerm* bterm) : ScanPin(bterm)
{
}

ScanDriver::ScanDriver(odb::dbITerm* iterm) : ScanPin(iterm)
{
}

ScanDriver::ScanDriver(odb::dbBTerm* bterm) : ScanPin(bterm)
{
}

}  // namespace dft
