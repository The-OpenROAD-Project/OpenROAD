#include "ScanCellMock.hh"

#include "ClockDomain.hh"

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
