#include "ScanPin.hh"

namespace dft {

ScanPin::ScanPin(Left<odb::dbITerm*>&& left) : Either(std::move(left))
{
}

ScanPin::ScanPin(Right<odb::dbBTerm*>&& right) : Either(std::move(right))
{
}

odb::dbNet* ScanPin::getNet() const
{
  switch (getSide()) {
    case EitherSide::Left:
      return left()->getNet();
      break;
    case EitherSide::Right:
      return right()->getNet();
      break;
  }
}

std::string_view ScanPin::getName() const
{
  switch (getSide()) {
    case EitherSide::Left:
      return left()->getMTerm()->getConstName();
      break;
    case EitherSide::Right:
      return right()->getConstName();
      break;
  }
}

ScanLoad::ScanLoad(Left<odb::dbITerm*>&& left) : ScanPin(std::move(left))
{
}

ScanLoad::ScanLoad(Right<odb::dbBTerm*>&& right) : ScanPin(std::move(right))
{
}

ScanDriver::ScanDriver(Left<odb::dbITerm*>&& left) : ScanPin(std::move(left))
{
}

ScanDriver::ScanDriver(Right<odb::dbBTerm*>&& right) : ScanPin(std::move(right))
{
}

}  // namespace dft
