#include "ScanCell.hh"
#pragma once

namespace dft {
namespace test {

class ScanCellMock : public ScanCell
{
 public:
  ScanCellMock(const std::string& name,
               std::unique_ptr<ClockDomain> clock_domain);
  ~ScanCellMock() override = default;

  uint64_t getBits() const override;
  void connectScanEnable() const override;
  void connectScanIn() const override;
  void connectScanOut() const override;
};

}  // namespace test
}  // namespace dft
