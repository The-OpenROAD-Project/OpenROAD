// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

odb::dbLib* read_lef(odb::dbDatabase* db, const char* path);

int write_lef(odb::dbLib* lib, const char* path);

int write_tech_lef(odb::dbTech* tech, const char* path);

int write_macro_lef(odb::dbLib* lib, const char* path);

odb::dbChip* read_def(odb::dbTech* tech, const std::string& path);

int write_def(odb::dbBlock* block,
              const char* path,
              odb::DefOut::Version version = odb::DefOut::Version::DEF_5_8);

odb::dbDatabase* read_db(odb::dbDatabase* db, const char* db_path);

int write_db(odb::dbDatabase* db, const char* db_path);

void createSBoxes(odb::dbSWire* swire,
                  odb::dbTechLayer* layer,
                  const std::vector<odb::Rect>& rects,
                  odb::dbWireShapeType type);

void createSBoxes(odb::dbSWire* swire,
                  odb::dbVia* via,
                  const std::vector<odb::Point>& points,
                  odb::dbWireShapeType type);

void dumpAPs(odb::dbBlock* block,
             const std::string file_name);
