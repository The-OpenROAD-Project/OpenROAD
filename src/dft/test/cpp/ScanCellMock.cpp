#include "ScanCellMock.hh"

#include "ClockDomain.hh"

namespace dft {
namespace test {

ScanCellMock::ScanCellMock(const std::string& name,
                           std::unique_ptr<ClockDomain> clock_domain)
    : ScanCell(name, std::move(clock_domain))
{
}

uint64_t ScanCellMock::getBits() const
{
  return 1;
}

void ScanCellMock::connectScanEnable() const
{
}

void ScanCellMock::connectScanIn() const
{
}

void ScanCellMock::connectScanOut() const
{
}

}  // namespace test
}  // namespace dft
