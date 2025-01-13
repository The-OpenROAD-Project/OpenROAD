#include "ScanPin.hh"

namespace dft {

ScanPin::ScanPin(std::variant<odb::dbBTerm*, odb::dbITerm*> term) : value_(term)
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

const std::variant<odb::dbBTerm*, odb::dbITerm*>& ScanPin::getValue() const
{
  return value_;
}

ScanLoad::ScanLoad(std::variant<odb::dbBTerm*, odb::dbITerm*> term)
    : ScanPin(term)
{
}

ScanDriver::ScanDriver(std::variant<odb::dbBTerm*, odb::dbITerm*> term)
    : ScanPin(term)
{
}

}  // namespace dft
