


void RepairPreCheck::checkSlewLimit(float ref_cap, float max_load_slew)
{
  // Ensure the max slew value specified is something the library can
  // potentially handle
  if (!best_case_slew_computed_ || ref_cap < best_case_slew_load_) {
    LibertyCellSeq *equiv_cells = sta_->equivCells(resizer_->buffer_lowest_drive_);
    float slew = bufferSlew(resizer_->buffer_lowest_drive_, ref_cap, resizer_->tgt_slew_dcalc_ap_);
    if (equiv_cells != nullptr) {
      for (LibertyCell *buffer: *equiv_cells) {
        slew = min(slew, bufferSlew(buffer, ref_cap, resizer_->tgt_slew_dcalc_ap_));
      }
    }
    best_case_slew_computed_ = true;
    best_case_slew_load_ = ref_cap;
    best_case_slew_ = slew;
  }

  if (max_load_slew < best_case_slew_) {
    logger_->error(RSZ, 90, "Max transition time from SDC is {}. Best achievable transition time is {} with {} load",
                   max_load_slew, best_case_slew_, best_case_slew_load_);
  }
}
