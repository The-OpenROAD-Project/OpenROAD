/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2022  EPFL
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*!
  \file aqfp_assumptions.hpp
  \brief Technology assumptions for AQFP

  \author Siang-Yun (Sonia) Lee
*/

#pragma once

namespace mockturtle
{

/*! \brief More realistic AQFP technology assumptions. */
struct aqfp_assumptions_realistic
{
  /*! \brief Whether CIs and COs need to be path-balanced. */
  bool balance_cios{ false };

  /*! \brief Ignores the complementations of COs because they can be merged into register inputs. */
  bool ignore_co_negation{ true };

  /*! \brief Number of phases per clock cycle (for phase alignment).
   *
   * Each CO (a node with external reference) must be scheduled at a level being a multiple of
   * `num_phases` (i.e., an imaginary CO node should be placed at a level `num_phases * k + 1`).
   */
  uint32_t num_phases{ 4u };

  /*! \brief The maximum number of fanouts a splitter/buffer can have. */
  uint32_t splitter_capacity{ 3u };

  /*! \brief The maximum number of fanouts a mega splitter can have. */
  //uint32_t mega_splitter_capacity{ 7u };

  /*! \brief The maximum number of fanouts a CI can have. */
  uint32_t ci_capacity{ 1u }; // simplicity
  //uint32_t ci_capacity{ 2u }; // best possible

  /*! \brief The phase offsets (after a change in register input) when new register output is available.
   *
   * Assumes that the register inputs (D and E) are scheduled at phase 0 (i.e., the last phase of
   * the previous clock cycle), a new state is available to be taken at these numbers of phases
   * afterwards.
   *
   * An ascending order is assumed. At least one element should be given.
   *
   * Each CI must be scheduled at a level `num_phases * k + ci_phases[i]` (for any `i`; for any
   * integer `k >= 0` when `balance_cios = false`, or `k=0` otherwise).
   */
  std::vector<uint32_t> ci_phases{ { 4u } }; // simplicity
  //std::vector<uint32_t> ci_phases{ { 3u, 4u, 5u } }; // best possible

  /*! \brief Maximum phase-skip (in consideration of clock skew). */
  uint32_t max_phase_skip{ 4u };
};

/*! \brief AQFP technology assumptions.
 *
 * POs count toward the fanout sizes and always have to be branched.
 * If PIs need to be balanced, then they must also need to be branched.
 */
struct aqfp_assumptions_legacy
{
  /*! \brief Whether PIs need to be branched with splitters. */
  bool branch_pis{ true };

  /*! \brief Whether PIs need to be path-balanced. */
  bool balance_pis{ false };

  /*! \brief Whether POs need to be path-balanced. */
  bool balance_pos{ false };

  /*! \brief The maximum number of fanouts each splitter (buffer) can have. */
  uint32_t splitter_capacity{ 3u };
};

using aqfp_assumptions = aqfp_assumptions_legacy;

/* Temporary helper function to bridge old and new code. */
inline aqfp_assumptions_realistic legacy_to_realistic( aqfp_assumptions_legacy const& legacy )
{
  aqfp_assumptions_realistic realistic;

  if ( !legacy.branch_pis )
  {
    realistic.ci_capacity = std::numeric_limits<uint32_t>::max();
  }
  else
  {
    realistic.ci_capacity = 1u;
  }

  if ( legacy.balance_pis && legacy.balance_pos )
  {
    realistic.balance_cios = true;
  }
  else if ( !legacy.balance_pis && !legacy.balance_pos )
  {
    realistic.balance_cios = false;
  }
  else
  {
    std::cerr << "[e] Cannot convert this combinaiton of assumptions.\n";
  }

  realistic.splitter_capacity = legacy.splitter_capacity;
  realistic.num_phases = 1u; // no phase alignment
  realistic.ci_phases = {0u}; // PIs at level 0
  realistic.max_phase_skip = std::numeric_limits<uint32_t>::max(); // no clock skew issue
  return realistic;
}

} // namespace mockturtle
