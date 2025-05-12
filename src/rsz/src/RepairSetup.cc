
      for (BaseMove* move : move_sequence) {
        debugPrint(logger_,
                   RSZ,
                   "moves",
                   2,
                   "Considering {} for {}",
                   move->name(),
                   network_->pathName(drvr_pin));

        if (move->doMove(drvr_path,
                         drvr_index,
                         path_slack,
                         &expanded,
                         setup_slack_margin)) {
          if (move == resizer_->unbuffer_move) {
            // Only allow one unbuffer move per pass to
            // prevent the use-after-free error of multiple buffer removals.
            changed += repairs_per_pass;
          } else {
            changed++;
          }
          // Move on to the next gate
          break;
        }
        debugPrint(logger_,
                   RSZ,
                   "moves",
                   2,
                   "Move {} failed for {}",
                   move->name(),
                   network_->pathName(drvr_pin));
      }
    }
  }
  return changed > 0;
}

void RepairSetup::printProgress(const int iteration,
                                const bool force,
                                const bool end,
                                const bool last_gasp,
                                const int num_viols) const
{
  const bool start = iteration == 0;

  if (start && !end) {
    logger_->report(
        "   Iter   | Removed | Resized | Inserted | Cloned |  Pin  |"
        "   Area   |    WNS   |   TNS      |  Viol  | Worst");
    logger_->report(
        "          | Buffers |  Gates  | Buffers  |  Gates | Swaps |"
        "          |          |            | Endpts | Endpt");
    logger_->report(
        "-----------------------------------------------------------"
        "---------------------------------------------------");
  }

  if (iteration % print_interval_ == 0 || force || end) {
    Slack wns;
    Vertex* worst_vertex;
    sta_->worstSlack(max_, wns, worst_vertex);
    const Slack tns = sta_->totalNegativeSlack(max_);

    std::string itr_field
        = fmt::format("{}{}", iteration, (last_gasp ? "*" : ""));
    if (end) {
      itr_field = "final";
    }

    const double design_area = resizer_->computeDesignArea();
    const double area_growth = design_area - initial_design_area_;

    // This actually prints both committed and pending moves, so the moves could
    // could go down if a pass is restrored by the journal.
    logger_->report(
        "{: >9s} | {: >7d} | {: >7d} | {: >8d} | {: >6d} | {: >5d} "
        "| {: >+7.1f}% | {: >8s} | {: >10s} | {: >6d} | {}",
        itr_field,
        resizer_->unbuffer_move->numMoves(),
        resizer_->size_move->numMoves(),
        resizer_->buffer_move->numMoves()
            + resizer_->split_load_move->numMoves(),
        resizer_->clone_move->numMoves(),
        resizer_->swap_pins_move->numMoves(),
        area_growth / initial_design_area_ * 1e2,
        delayAsString(wns, sta_, 3),
        delayAsString(tns, sta_, 1),
        max(0, num_viols),
        worst_vertex != nullptr ? worst_vertex->name(network_) : "");
  }

  if (end) {
    logger_->report(
        "-----------------------------------------------------------"
        "---------------------------------------------------");
  }
}

// Terminate progress if incremental fix rate within an opto interval falls
// below the threshold.   Bump up the threshold after each large opto
// interval.
bool RepairSetup::terminateProgress(const int iteration,
                                    const float initial_tns,
                                    float& prev_tns,
                                    float& fix_rate_threshold,
                                    // for debug only
                                    const int endpt_index,
                                    const int num_endpts)
{
  if (iteration % opto_large_interval_ == 0) {
    fix_rate_threshold *= 2.0;
  }
  if (iteration % opto_small_interval_ == 0) {
    float curr_tns = sta_->totalNegativeSlack(max_);
    float inc_fix_rate = (prev_tns - curr_tns) / initial_tns;
    prev_tns = curr_tns;
    if (iteration > 1000  // allow for some initial fixing for 1000 iterations
        && inc_fix_rate < fix_rate_threshold) {
      // clang-format off
      debugPrint(logger_, RSZ, "repair_setup", 1, "bailing out at iter {}"
                 " because incr fix rate {:0.2f}% is < {:0.2f}% [endpt {}/{}]",
                 iteration, inc_fix_rate*100, fix_rate_threshold*100,
                 endpt_index, num_endpts);
      // clang-format on
      return true;
    }
  }
  return false;
}

// Perform some last fixing based on sizing only.
// This is a greedy opto that does not degrade WNS or TNS.
// TODO: add VT swap
void RepairSetup::repairSetupLastGasp(const OptoParams& params, int& num_viols)
{
  // Sort remaining failing endpoints
  const VertexSet* endpoints = sta_->endpoints();
  vector<pair<Vertex*, Slack>> violating_ends;
  for (Vertex* end : *endpoints) {
    const Slack end_slack = sta_->vertexSlack(end, max_);
    if (end_slack < params.setup_slack_margin) {
      violating_ends.emplace_back(end, end_slack);
    }
  }
  std::stable_sort(violating_ends.begin(),
                   violating_ends.end(),
                   [](const auto& end_slack1, const auto& end_slack2) {
                     return end_slack1.second < end_slack2.second;
                   });
  num_viols = violating_ends.size();

  float curr_tns = sta_->totalNegativeSlack(max_);
  if (fuzzyGreaterEqual(curr_tns, 0)) {
    // clang-format off
    debugPrint(logger_, RSZ, "repair_setup", 1, "last gasp is bailing out "
               "because TNS is {:0.2f}", curr_tns);
    // clang-format on
    return;
  }

  // Don't do anything unless there was some progress from previous fixing
  if ((params.initial_tns - curr_tns) / params.initial_tns < 0.05) {
    // clang-format off
    debugPrint(logger_, RSZ, "repair_setup", 1, "last gasp is bailing out "
               "because TNS was reduced by < 5% from previous fixing");
    // clang-format on
    return;
  }

  int end_index = 0;
  int max_end_count = violating_ends.size();
  if (max_end_count == 0) {
    // clang-format off
    debugPrint(logger_, RSZ, "repair_setup", 1, "last gasp is bailing out "
               "because there are no violating endpoints");
    // clang-format on
    return;
  }
  // clang-format off
  debugPrint(logger_, RSZ, "repair_setup", 1, "{} violating endpoints remain",
             max_end_count);
  // clang-format on
  int opto_iteration = params.iteration;
  printProgress(opto_iteration, false, false, true, num_viols);

  float prev_tns = curr_tns;
  Slack curr_worst_slack = violating_ends[0].second;
  Slack prev_worst_slack = curr_worst_slack;
  bool prev_termination = false;
  bool two_cons_terminations = false;
  float fix_rate_threshold = inc_fix_rate_threshold_;

  for (const auto& end_original_slack : violating_ends) {
    fallback_ = false;
    Vertex* end = end_original_slack.first;
    Slack end_slack = sta_->vertexSlack(end, max_);
    Slack worst_slack;
    Vertex* worst_vertex;
    sta_->worstSlack(max_, worst_slack, worst_vertex);
    end_index++;
    if (end_index > max_end_count) {
      break;
    }
    int pass = 1;
    resizer_->journalBegin();
    while (pass <= max_last_gasp_passes_) {
      opto_iteration++;
      if (terminateProgress(opto_iteration,
                            params.initial_tns,
                            prev_tns,
                            fix_rate_threshold,
                            end_index,
                            max_end_count)) {
        if (prev_termination) {
          // Abort entire fixing if no progress for 200 iterations
          two_cons_terminations = true;
        } else {
          prev_termination = true;
        }
        resizer_->journalEnd();
        break;
      }
      if (opto_iteration % opto_small_interval_ == 0) {
        prev_termination = false;
      }
      if (params.verbose || opto_iteration == 1) {
        printProgress(opto_iteration, false, false, true, num_viols);
      }
      if (end_slack > params.setup_slack_margin) {
        --num_viols;
        resizer_->journalEnd();
        break;
      }
      Path* end_path = sta_->vertexWorstSlackPath(end, max_);

      const bool changed
          = repairPath(end_path, end_slack, params.setup_slack_margin);

      if (!changed) {
        if (pass != 1) {
          resizer_->journalRestore();
        } else {
          resizer_->journalEnd();
        }
        break;
      }
      resizer_->updateParasitics();
      sta_->findRequireds();
      end_slack = sta_->vertexSlack(end, max_);
      sta_->worstSlack(max_, curr_worst_slack, worst_vertex);
      curr_tns = sta_->totalNegativeSlack(max_);

      // Accept only moves that improve both WNS and TNS
      if (fuzzyGreaterEqual(curr_worst_slack, prev_worst_slack)
          && fuzzyGreaterEqual(curr_tns, prev_tns)) {
        // clang-format off
        debugPrint(logger_, RSZ, "repair_setup", 1, "sizing move accepted for "
                   "endpoint {} pass {} because WNS improved to {:0.3f} and "
                   "TNS improved to {:0.3f}",
                   end_index, pass, curr_worst_slack, curr_tns);
        // clang-format on
        prev_worst_slack = curr_worst_slack;
        prev_tns = curr_tns;
        if (end_slack > params.setup_slack_margin) {
          --num_viols;
        }
        resizer_->journalEnd();
        resizer_->journalBegin();
      } else {
        fallback_ = true;
        resizer_->journalRestore();
        break;
      }

      if (resizer_->overMaxArea()) {
        resizer_->journalEnd();
        break;
      }
      if (end_index == 1) {
        end = worst_vertex;
      }
      pass++;
    }  // while pass <= max_last_gasp_passes_
    if (params.verbose || opto_iteration == 1) {
      printProgress(opto_iteration, true, false, true, num_viols);
    }
    if (two_cons_terminations) {
      // clang-format off
      debugPrint(logger_, RSZ, "repair_setup", 1, "bailing out of last gasp fixing"
                 "due to no TNS progress for two opto cycles");
      // clang-format on
      break;
    }
  }  // for each violating endpoint
}

}  // namespace rsz
