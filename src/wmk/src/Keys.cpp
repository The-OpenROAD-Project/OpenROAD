// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "Keys.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "HmacSha256.h"

namespace wmk {

namespace {

const char* const kStages[] = {"placement", "cts", "routing"};

}  // namespace

std::array<std::uint8_t, 32> deriveStageKey(
    const std::array<std::uint8_t, 32>& master,
    const std::string& design_id,
    const std::vector<std::uint8_t>& nonce,
    const std::string& stage)
{
  const std::string nonce_str(nonce.begin(), nonce.end());
  return hmac_digest(master, {design_id, nonce_str, "stage=" + stage});
}

bool isWatermarkStage(const std::string& stage)
{
  for (const char* s : kStages) {
    if (stage == s) {
      return true;
    }
  }
  return false;
}

}  // namespace wmk
