#include "cells/ScanCell.hh"
#pragma once

namespace dft {
namespace test {

class ScanCellMock : public ScanCell
{
 public:
  ScanCellMock(const std::string& name, ClockDomain clock_domain);
  ~ScanCellMock() override = default;

  uint64_t getBits() const override;
  void connectSE() const override;
  void connectSI() const override;
  void connectSO() const override;
};

}  // namespace test
}  // namespace dft
