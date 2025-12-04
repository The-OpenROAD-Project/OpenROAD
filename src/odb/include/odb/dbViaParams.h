// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "odb/dbId.h"
#include "odb/odb.h"

namespace odb {

class _dbTechLayer;
class _dbDatabase;
class dbIStream;
class dbOStream;

class _dbViaParams
{
 public:
  _dbViaParams(const _dbViaParams& v);
  _dbViaParams();
  ~_dbViaParams();

  bool operator==(const _dbViaParams& rhs) const;
  bool operator!=(const _dbViaParams& rhs) const { return !operator==(rhs); }
  friend dbOStream& operator<<(dbOStream& stream, const _dbViaParams& v);
  friend dbIStream& operator>>(dbIStream& stream, _dbViaParams& v);

  int x_cut_size_;
  int y_cut_size_;
  int x_cut_spacing_;
  int y_cut_spacing_;
  int x_top_enclosure_;
  int y_top_enclosure_;
  int x_bot_enclosure_;
  int y_bot_enclosure_;
  int num_cut_rows_;
  int num_cut_cols_;
  int x_origin_;
  int y_origin_;
  int x_top_offset_;
  int y_top_offset_;
  int x_bot_offset_;
  int y_bot_offset_;
  dbId<_dbTechLayer> top_layer_;
  dbId<_dbTechLayer> cut_layer_;
  dbId<_dbTechLayer> bot_layer_;
};

dbOStream& operator<<(dbOStream& stream, const _dbViaParams& v);
dbIStream& operator>>(dbIStream& stream, _dbViaParams& v);

}  // namespace odb
