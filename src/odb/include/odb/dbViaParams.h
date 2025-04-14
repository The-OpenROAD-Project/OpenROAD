// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbId.h"
#include "odb.h"

namespace odb {

class _dbTechLayer;
class _dbDatabase;
class dbIStream;
class dbOStream;

class _dbViaParams
{
 public:
  int _x_cut_size;
  int _y_cut_size;
  int _x_cut_spacing;
  int _y_cut_spacing;
  int _x_top_enclosure;
  int _y_top_enclosure;
  int _x_bot_enclosure;
  int _y_bot_enclosure;
  int _num_cut_rows;
  int _num_cut_cols;
  int _x_origin;
  int _y_origin;
  int _x_top_offset;
  int _y_top_offset;
  int _x_bot_offset;
  int _y_bot_offset;
  dbId<_dbTechLayer> _top_layer;
  dbId<_dbTechLayer> _cut_layer;
  dbId<_dbTechLayer> _bot_layer;

  _dbViaParams(const _dbViaParams& v);
  _dbViaParams();
  ~_dbViaParams();

  bool operator==(const _dbViaParams& rhs) const;
  bool operator!=(const _dbViaParams& rhs) const { return !operator==(rhs); }
  friend dbOStream& operator<<(dbOStream& stream, const _dbViaParams& v);
  friend dbIStream& operator>>(dbIStream& stream, _dbViaParams& v);
};

dbOStream& operator<<(dbOStream& stream, const _dbViaParams& v);
dbIStream& operator>>(dbIStream& stream, _dbViaParams& v);

}  // namespace odb
