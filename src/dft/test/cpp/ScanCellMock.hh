#include "cells/ScanCell.hh"
#pragma once


namespace dft {
namespace test {

class ScanCellMock: public ScanCell {
 public:
  ScanCellMock(std::string name, ClockDomain clock_domain);
  virtual ~ScanCellMock() = default;

  virtual uint64_t getBits() const override;
  virtual void connectSE() const override;
  virtual void connectSI() const override;
  virtual void connectSO() const override;
};

} // namespace test
} // namespace dft
