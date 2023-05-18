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
  // unreachable code. There are only two values in the EitherSide enum. This
  // prevents warnings and compilation errors: See https://abseil.io/tips/147
  return nullptr;
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
  // unreachable code. There are only two values in the EitherSide enum. This
  // prevents warnings and compilation errors: See https://abseil.io/tips/147
  return {};
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
