// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#pragma once

namespace gpl {
class Replace;
}

namespace ord {

class OpenRoad;

gpl::Replace* makeReplace();

void initReplace(OpenRoad* openroad);

void deleteReplace(gpl::Replace* replace);

}  // namespace ord
