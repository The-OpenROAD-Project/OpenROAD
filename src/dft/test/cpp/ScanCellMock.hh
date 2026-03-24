#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "ScanCell.hh"
#include "ScanPin.hh"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace dft {
namespace test {

class ScanCellMock : public ScanCell
{
 public:
  ScanCellMock(const std::string& name,
               std::unique_ptr<ClockDomain> clock_domain,
               utl::Logger* logger);
  ~ScanCellMock() override = default;

  uint64_t getBits() const override;
  void connectScanEnable(const ScanDriver& pin) const override;
  void connectScanIn(const ScanDriver& pin) const override;
  void connectScanOut(const ScanLoad& pin) const override;
  ScanLoad getScanEnable() const override;
  ScanLoad getScanIn() const override;
  ScanDriver getScanOut() const override;
  odb::Point getOrigin() const override;
  bool isPlaced() const override;
};

}  // namespace test
}  // namespace dft
