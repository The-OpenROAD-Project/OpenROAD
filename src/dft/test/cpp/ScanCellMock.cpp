#include "ScanCellMock.hh"

namespace dft {
namespace test {

ScanCellMock::ScanCellMock(std::string name, ClockDomain clock_domain)
    : ScanCell(name, std::move(clock_domain))
{
}

uint64_t ScanCellMock::getBits() const
{
  return 1;
}

void ScanCellMock::connectSE() const
{
}

void ScanCellMock::connectSI() const
{
}

void ScanCellMock::connectSO() const
{
}

}  // namespace test
}  // namespace dft
