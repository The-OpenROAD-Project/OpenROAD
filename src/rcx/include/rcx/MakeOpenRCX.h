// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

namespace rcx {
class Ext;
}  // namespace rcx

namespace ord {

class OpenRoad;

rcx::Ext* makeOpenRCX();

void deleteOpenRCX(rcx::Ext* extractor);

void initOpenRCX(OpenRoad* openroad);

}  // namespace ord
