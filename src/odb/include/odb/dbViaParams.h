// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "odb/dbId.h"

namespace odb {

class _dbTechLayer;
class _dbDatabase;
class dbIStream;
class dbOStream;

class _dbViaParams
{
 public:
  _dbViaParams(const _dbViaParams& v) = default;
  _dbViaParams() = default;

  bool operator==(const _dbViaParams& rhs) const;
  bool operator!=(const _dbViaParams& rhs) const { return !operator==(rhs); }
  friend dbOStream& operator<<(dbOStream& stream, const _dbViaParams& v);
  friend dbIStream& operator>>(dbIStream& stream, _dbViaParams& v);

  int x_cut_size_{0};
  int y_cut_size_{0};
  int x_cut_spacing_{0};
  int y_cut_spacing_{0};
  int x_top_enclosure_{0};
  int y_top_enclosure_{0};
  int x_bot_enclosure_{0};
  int y_bot_enclosure_{0};
  int num_cut_rows_{1};
  int num_cut_cols_{1};
  int x_origin_{0};
  int y_origin_{0};
  int x_top_offset_{0};
  int y_top_offset_{0};
  int x_bot_offset_{0};
  int y_bot_offset_{0};
  dbId<_dbTechLayer> top_layer_;
  dbId<_dbTechLayer> cut_layer_;
  dbId<_dbTechLayer> bot_layer_;
};

dbOStream& operator<<(dbOStream& stream, const _dbViaParams& v);
dbIStream& operator>>(dbIStream& stream, _dbViaParams& v);

}  // namespace odb
