#include "Stitch.hh"

namespace dft {
namespace stitch {

Stitch::Stitch() {
}

bool Action::IsDone() const {
  return done_;
}

ScanReplace::ScanReplace(sta::LibertyCell* original_cell):
  original_cell_(original_cell),
  new_cell_(nullptr)
{ }

bool ScanReplace::Do(State &state) {
  return true;
}

} // namespace stitch
} // namespace dft

