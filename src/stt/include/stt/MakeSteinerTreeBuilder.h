// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

namespace stt {
class SteinerTreeBuilder;
}

namespace ord {

class OpenRoad;

stt::SteinerTreeBuilder* makeSteinerTreeBuilder();

void initSteinerTreeBuilder(OpenRoad* openroad);

void deleteSteinerTreeBuilder(stt::SteinerTreeBuilder* stt_builder);

}  // namespace ord
