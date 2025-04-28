// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

%apply std::vector<odb::dbShape> &OUTPUT { std::vector<odb::dbShape> & shapes };

// (dbGCellGrid|dbTrackGrid)::getGridPattern[XY]
%apply int& OUTPUT { int& origin_x, int& origin_y, int& line_count, int& step };
%apply int& OUTPUT { int& origin_x, int& origin_y, int& line_count, int& step, int& first_mask, bool& samemask };

// dbPowerDomain::getArea
%apply odb::Rect& OUTPUT { odb::Rect& area };

// (dbGCellGrid|dbTrackGrid)::getGrid[XY]
%typemap(in, numinputs=0) std::vector<int> &OUTPUT  {
   $1 = new std::vector<int>();
}

%apply std::vector<int> &OUTPUT { std::vector<int> & x_grid,
                                  std::vector<int> & y_grid };
