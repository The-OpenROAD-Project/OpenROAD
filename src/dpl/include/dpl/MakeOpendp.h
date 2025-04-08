// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

namespace dpl {
class Opendp;
}

namespace ord {

class OpenRoad;

dpl::Opendp* makeOpendp();
void initOpendp(OpenRoad* openroad);
void deleteOpendp(dpl::Opendp* opendp);

}  // namespace ord
