#include "ScanCellMock.hh"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "ClockDomain.hh"
#include "ScanCell.hh"
#include "ScanPin.hh"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace dft {
namespace test {

ScanCellMock::ScanCellMock(const std::string& name,
                           std::unique_ptr<ClockDomain> clock_domain,
                           utl::Logger* logger)
    : ScanCell(name, std::move(clock_domain), logger)
{
}

uint64_t ScanCellMock::getBits() const
{
  return 1;
}

void ScanCellMock::connectScanEnable(const ScanDriver& pin) const
{
}

void ScanCellMock::connectScanIn(const ScanDriver& pin) const
{
}

void ScanCellMock::connectScanOut(const ScanLoad& pin) const
{
}

ScanLoad ScanCellMock::getScanEnable() const
{
  return ScanLoad(static_cast<odb::dbITerm*>(nullptr));
}

ScanLoad ScanCellMock::getScanIn() const
{
  return ScanLoad(static_cast<odb::dbBTerm*>(nullptr));
}

ScanDriver ScanCellMock::getScanOut() const
{
  return ScanDriver(static_cast<odb::dbBTerm*>(nullptr));
}

odb::Point ScanCellMock::getOrigin() const
{
  return odb::Point();
}

bool ScanCellMock::isPlaced() const
{
  return false;
}

}  // namespace test
}  // namespace dft
