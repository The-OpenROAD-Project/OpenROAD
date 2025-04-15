// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

namespace sta {
class dbSta;
}

namespace ord {

class OpenRoad;

sta::dbSta* makeDbSta();
void deleteDbSta(sta::dbSta* sta);
void initDbSta(OpenRoad* openroad);

}  // namespace ord
