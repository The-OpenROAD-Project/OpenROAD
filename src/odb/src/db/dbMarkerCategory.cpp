///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// Generator Code Begin Cpp
#include "dbMarkerCategory.h"

#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbMarker.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbSet.h"
// User Code Begin Includes
#include <boost/regex.hpp>
#include <fstream>
#include <regex>
#include <sstream>

#include "dbHashTable.hpp"
#include "odb/dbBlockCallBackObj.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbMarkerCategory>;

bool _dbMarkerCategory::operator==(const _dbMarkerCategory& rhs) const
{
  if (_name != rhs._name) {
    return false;
  }
  if (description_ != rhs.description_) {
    return false;
  }
  if (source_ != rhs.source_) {
    return false;
  }
  if (max_markers_ != rhs.max_markers_) {
    return false;
  }
  if (*marker_tbl_ != *rhs.marker_tbl_) {
    return false;
  }
  if (*categories_tbl_ != *rhs.categories_tbl_) {
    return false;
  }
  if (categories_hash_ != rhs.categories_hash_) {
    return false;
  }
  if (_next_entry != rhs._next_entry) {
    return false;
  }

  return true;
}

bool _dbMarkerCategory::operator<(const _dbMarkerCategory& rhs) const
{
  return true;
}

void _dbMarkerCategory::differences(dbDiff& diff,
                                    const char* field,
                                    const _dbMarkerCategory& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_name);
  DIFF_FIELD(description_);
  DIFF_FIELD(source_);
  DIFF_FIELD(max_markers_);
  DIFF_TABLE(marker_tbl_);
  DIFF_TABLE(categories_tbl_);
  DIFF_HASH_TABLE(categories_hash_);
  DIFF_FIELD_NO_DEEP(_next_entry);
  DIFF_END
}

void _dbMarkerCategory::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(description_);
  DIFF_OUT_FIELD(source_);
  DIFF_OUT_FIELD(max_markers_);
  DIFF_OUT_TABLE(marker_tbl_);
  DIFF_OUT_TABLE(categories_tbl_);
  DIFF_OUT_HASH_TABLE(categories_hash_);
  DIFF_OUT_FIELD_NO_DEEP(_next_entry);

  DIFF_END
}

_dbMarkerCategory::_dbMarkerCategory(_dbDatabase* db)
{
  _name = nullptr;
  max_markers_ = 10000;
  marker_tbl_ = new dbTable<_dbMarker>(
      db, this, (GetObjTbl_t) &_dbMarkerCategory::getObjectTable, dbMarkerObj);
  categories_tbl_ = new dbTable<_dbMarkerCategory>(
      db,
      this,
      (GetObjTbl_t) &_dbMarkerCategory::getObjectTable,
      dbMarkerCategoryObj);
  categories_hash_.setTable(categories_tbl_);
}

_dbMarkerCategory::_dbMarkerCategory(_dbDatabase* db,
                                     const _dbMarkerCategory& r)
{
  _name = r._name;
  description_ = r.description_;
  source_ = r.source_;
  max_markers_ = r.max_markers_;
  marker_tbl_ = new dbTable<_dbMarker>(db, this, *r.marker_tbl_);
  categories_tbl_
      = new dbTable<_dbMarkerCategory>(db, this, *r.categories_tbl_);
  categories_hash_.setTable(categories_tbl_);
  _next_entry = r._next_entry;
}

dbIStream& operator>>(dbIStream& stream, _dbMarkerCategory& obj)
{
  stream >> obj._name;
  stream >> obj.description_;
  stream >> obj.source_;
  stream >> obj.max_markers_;
  stream >> *obj.marker_tbl_;
  stream >> *obj.categories_tbl_;
  stream >> obj.categories_hash_;
  stream >> obj._next_entry;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbMarkerCategory& obj)
{
  stream << obj._name;
  stream << obj.description_;
  stream << obj.source_;
  stream << obj.max_markers_;
  stream << *obj.marker_tbl_;
  stream << *obj.categories_tbl_;
  stream << obj.categories_hash_;
  stream << obj._next_entry;
  return stream;
}

dbObjectTable* _dbMarkerCategory::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbMarkerObj:
      return marker_tbl_;
    case dbMarkerCategoryObj:
      return categories_tbl_;
    default:
      break;
  }
  return getTable()->getObjectTable(type);
}

_dbMarkerCategory::~_dbMarkerCategory()
{
  if (_name) {
    free((void*) _name);
  }
  delete marker_tbl_;
  delete categories_tbl_;
}

// User Code Begin PrivateMethods

bool _dbMarkerCategory::isTopCategory() const
{
  return getOwner()->getObjectType() == dbBlockObj;
}

_dbBlock* _dbMarkerCategory::getBlock() const
{
  dbMarkerCategory* category = (dbMarkerCategory*) this;
  _dbMarkerCategory* top_category
      = (_dbMarkerCategory*) category->getTopCategory();
  return (_dbBlock*) top_category->getOwner();
}

bool _dbMarkerCategory::hasMaxMarkerLimit() const
{
  return max_markers_ > 0;
}

void _dbMarkerCategory::populatePTree(
    _dbMarkerCategory::PropertyTree& tree) const
{
  dbMarkerCategory* category = (dbMarkerCategory*) this;

  _dbMarkerCategory::PropertyTree category_tree;

  category_tree.put("description", category->getDescription());
  category_tree.put("source", category->getSource());
  if (hasMaxMarkerLimit()) {
    category_tree.put("max_markers", category->getMaxMarkers());
  }

  _dbMarkerCategory::PropertyTree subcategory_tree;
  for (dbMarkerCategory* category : category->getMarkerCategorys()) {
    _dbMarkerCategory* category_ = (_dbMarkerCategory*) category;
    category_->populatePTree(subcategory_tree);
  }
  if (!category->getMarkerCategorys().empty()) {
    category_tree.add_child("category", subcategory_tree);
  }

  _dbMarkerCategory::PropertyTree violations_tree;
  for (dbMarker* marker : category->getMarkers()) {
    _dbMarker* marker_ = (_dbMarker*) marker;
    marker_->populatePTree(violations_tree);
  }
  if (!category->getMarkers().empty()) {
    category_tree.add_child("violations", violations_tree);
  }

  // Since . is the default separator we may need to pick something else
  char separator = '.';
  const std::string key(category->getName());
  while (key.find(separator) != std::string::npos) {
    separator += 1;
  }
  tree.add_child(_dbMarkerCategory::PropertyTree::path_type{key, separator},
                 category_tree);
}

void _dbMarkerCategory::fromPTree(const PropertyTree& tree)
{
  description_ = tree.get<std::string>("description", "");
  source_ = tree.get<std::string>("source", "");
  max_markers_ = tree.get<int>("max_markers", 0);

  auto child_category = tree.get_child_optional("category");
  if (child_category) {
    for (const auto& [name, subtree] : child_category.value()) {
      dbMarkerCategory* category = dbMarkerCategory::createOrReplace(
          (dbMarkerCategory*) this, name.c_str());
      _dbMarkerCategory* category_ = (_dbMarkerCategory*) category;

      category_->fromPTree(subtree);
    }
  }

  auto child_violations = tree.get_child_optional("violations");
  if (child_violations) {
    for (const auto& [_, subtree] : child_violations.value()) {
      dbMarker* marker = dbMarker::create((dbMarkerCategory*) this);
      if (marker == nullptr) {
        continue;
      }

      _dbMarker* marker_ = (_dbMarker*) marker;
      marker_->fromPTree(subtree);
    }
  }
}

void _dbMarkerCategory::writeJSON(
    std::ofstream& report,
    const std::set<_dbMarkerCategory*>& categories)
{
  if (categories.empty()) {
    return;
  }

  std::set<dbMarkerCategory*> ordered_categories;
  for (_dbMarkerCategory* category : categories) {
    ordered_categories.insert((dbMarkerCategory*) category);
  }

  _dbMarkerCategory::PropertyTree tree;
  for (dbMarkerCategory* category : ordered_categories) {
    _dbMarkerCategory* category_ = (_dbMarkerCategory*) category;
    category_->populatePTree(tree);
  }

  try {
    boost::property_tree::json_parser::write_json(report, tree);
  } catch (const boost::property_tree::json_parser_error& e1) {
    _dbMarkerCategory* _top_category
        = (_dbMarkerCategory*) (*ordered_categories.begin())->getTopCategory();
    _dbBlock* block = (_dbBlock*) _top_category->getOwner();
    utl::Logger* logger = block->getLogger();

    logger->error(utl::ODB, 268, "Unable to write markers: {}", e1.what());
  }
}

void _dbMarkerCategory::writeTR(std::ofstream& report) const
{
  dbMarkerCategory* marker_category = (dbMarkerCategory*) this;

  for (dbMarker* marker : marker_category->getMarkers()) {
    _dbMarker* marker_ = (_dbMarker*) marker;
    marker_->writeTR(report);
  }

  for (dbMarkerCategory* category : marker_category->getMarkerCategorys()) {
    _dbMarkerCategory* category_ = (_dbMarkerCategory*) category;
    category_->writeTR(report);
  }
}

// User Code End PrivateMethods

////////////////////////////////////////////////////////////////////
//
// dbMarkerCategory - Methods
//
////////////////////////////////////////////////////////////////////

const char* dbMarkerCategory::getName() const
{
  _dbMarkerCategory* obj = (_dbMarkerCategory*) this;
  return obj->_name;
}

void dbMarkerCategory::setDescription(const std::string& description)
{
  _dbMarkerCategory* obj = (_dbMarkerCategory*) this;

  obj->description_ = description;
}

std::string dbMarkerCategory::getDescription() const
{
  _dbMarkerCategory* obj = (_dbMarkerCategory*) this;
  return obj->description_;
}

void dbMarkerCategory::setSource(const std::string& source)
{
  _dbMarkerCategory* obj = (_dbMarkerCategory*) this;

  obj->source_ = source;
}

void dbMarkerCategory::setMaxMarkers(int max_markers)
{
  _dbMarkerCategory* obj = (_dbMarkerCategory*) this;

  obj->max_markers_ = max_markers;
}

int dbMarkerCategory::getMaxMarkers() const
{
  _dbMarkerCategory* obj = (_dbMarkerCategory*) this;
  return obj->max_markers_;
}

dbSet<dbMarker> dbMarkerCategory::getMarkers() const
{
  _dbMarkerCategory* obj = (_dbMarkerCategory*) this;
  return dbSet<dbMarker>(obj, obj->marker_tbl_);
}

dbSet<dbMarkerCategory> dbMarkerCategory::getMarkerCategorys() const
{
  _dbMarkerCategory* obj = (_dbMarkerCategory*) this;
  return dbSet<dbMarkerCategory>(obj, obj->categories_tbl_);
}

dbMarkerCategory* dbMarkerCategory::findMarkerCategory(const char* name) const
{
  _dbMarkerCategory* obj = (_dbMarkerCategory*) this;
  return (dbMarkerCategory*) obj->categories_hash_.find(name);
}

// User Code Begin dbMarkerCategoryPublicMethods

int dbMarkerCategory::getMarkerCount() const
{
  _dbMarkerCategory* obj = (_dbMarkerCategory*) this;
  int count = obj->marker_tbl_->size();

  for (dbMarkerCategory* category : getMarkerCategorys()) {
    count += category->getMarkerCount();
  }

  return count;
}

dbMarkerCategory* dbMarkerCategory::getTopCategory() const
{
  _dbMarkerCategory* top = (_dbMarkerCategory*) this;
  while (!top->isTopCategory()) {
    top = (_dbMarkerCategory*) top->getOwner();
  }
  return (dbMarkerCategory*) top;
}

dbObject* dbMarkerCategory::getParent() const
{
  _dbMarkerCategory* obj = (_dbMarkerCategory*) this;
  return obj->getOwner();
}

std::string dbMarkerCategory::getSource() const
{
  _dbMarkerCategory* obj = (_dbMarkerCategory*) this;
  if (!obj->source_.empty() || obj->isTopCategory()) {
    return obj->source_;
  }

  return getTopCategory()->getSource();
}

bool dbMarkerCategory::rename(const char* name)
{
  _dbMarkerCategory* _category = (_dbMarkerCategory*) this;

  if (_category->isTopCategory()) {
    _dbBlock* block = (_dbBlock*) _category->getOwner();

    if (block->_marker_category_hash.hasMember(name)) {
      return false;
    }

    block->_marker_category_hash.remove(_category);

    free((void*) _category->_name);
    _category->_name = strdup(name);
    ZALLOCATED(_category->_name);

    block->_marker_category_hash.insert(_category);
  } else {
    _dbMarkerCategory* parent = (_dbMarkerCategory*) _category->getOwner();

    if (parent->categories_hash_.hasMember(name)) {
      return false;
    }

    parent->categories_hash_.remove(_category);

    free((void*) _category->_name);
    _category->_name = strdup(name);
    ZALLOCATED(_category->_name);

    parent->categories_hash_.insert(_category);
  }

  return true;
}

void dbMarkerCategory::writeJSON(const std::string& path) const
{
  std::ofstream report(path);

  if (!report) {
    _dbMarkerCategory* obj = (_dbMarkerCategory*) this;
    utl::Logger* logger = obj->getLogger();

    logger->error(utl::ODB, 281, "Unable to open {} to write markers", path);
  }

  writeJSON(report);

  report.close();
}

void dbMarkerCategory::writeJSON(std::ofstream& report) const
{
  _dbMarkerCategory* obj = (_dbMarkerCategory*) this;
  _dbMarkerCategory::writeJSON(report, {obj});
}

void dbMarkerCategory::writeTR(const std::string& path) const
{
  std::ofstream report(path);

  if (!report) {
    _dbMarkerCategory* obj = (_dbMarkerCategory*) this;
    utl::Logger* logger = obj->getLogger();

    logger->error(utl::ODB, 269, "Unable to open {} to write markers", path);
  }

  writeTR(report);

  report.close();
}

void dbMarkerCategory::writeTR(std::ofstream& report) const
{
  _dbMarkerCategory* obj = (_dbMarkerCategory*) this;

  obj->writeTR(report);
}

std::set<dbMarkerCategory*> dbMarkerCategory::fromJSON(dbBlock* block,
                                                       const std::string& path)
{
  std::ifstream report(path);
  if (!report.is_open()) {
    _dbBlock* _block = (_dbBlock*) block;
    utl::Logger* logger = _block->getLogger();

    logger->error(utl::ODB, 31, "Unable to open marker report: {}", path);
  }

  std::set<dbMarkerCategory*> categories
      = fromJSON(block, path.c_str(), report);

  report.close();

  return categories;
}

std::set<dbMarkerCategory*> dbMarkerCategory::fromJSON(dbBlock* block,
                                                       const char* source,
                                                       std::ifstream& report)
{
  _dbBlock* _block = (_dbBlock*) block;
  utl::Logger* logger = _block->getLogger();

  _dbMarkerCategory::PropertyTree tree;
  try {
    boost::property_tree::json_parser::read_json(report, tree);
  } catch (const boost::property_tree::json_parser_error& e1) {
    logger->error(utl::ODB, 238, "Unable to parse JSON file: {}", e1.what());
  }

  std::set<dbMarkerCategory*> categories;
  for (const auto& [name, subtree] : tree) {
    dbMarkerCategory* top_category
        = dbMarkerCategory::createOrReplace(block, name.c_str());
    categories.insert(top_category);
    _dbMarkerCategory* top_category_ = (_dbMarkerCategory*) top_category;

    top_category_->fromPTree(subtree);
  }

  return categories;
}

dbMarkerCategory* dbMarkerCategory::fromTR(dbBlock* block,
                                           const char* name,
                                           const std::string& path)
{
  std::ifstream report(path);
  if (!report.is_open()) {
    _dbBlock* _block = (_dbBlock*) block;
    utl::Logger* logger = _block->getLogger();

    logger->error(
        utl::ODB, 30, "Unable to open TritonRoute DRC report: {}", path);
  }

  dbMarkerCategory* category = fromTR(block, name, path.c_str(), report);

  report.close();

  return category;
}

dbMarkerCategory* dbMarkerCategory::fromTR(dbBlock* block,
                                           const char* name,
                                           const char* source,
                                           std::ifstream& report)
{
  dbMarkerCategory* marker_category = createOrReplace(block, name);
  marker_category->setSource(source);

  _dbBlock* _block = (_dbBlock*) block;
  utl::Logger* logger = _block->getLogger();

  const std::regex violation_type("\\s*violation type: (.*)");
  const boost::regex srcs("\\s*srcs: (.*)");
  const std::regex comment_line("\\s*(comment|congestion information): (.*)");
  const std::regex bbox_layer("\\s*bbox = (.*) on Layer (.*)");
  const std::regex bbox_corners(
      "\\s*\\(\\s*(.*),\\s*(.*)\\s*\\)\\s*-\\s*\\(\\s*(.*),\\s*(.*)\\s*\\)");

  int line_number = 0;
  dbTech* tech = block->getTech();
  while (!report.eof()) {
    std::string line;
    std::smatch base_match;

    // type of violation
    line_number++;
    std::getline(report, line);
    if (line.empty()) {
      continue;
    }

    int violation_line_number = line_number;
    std::string type;
    if (std::regex_match(line, base_match, violation_type)) {
      type = base_match[1].str();
    } else {
      logger->error(utl::ODB,
                    55,
                    "Unable to parse line as violation type (line: {}): {}",
                    line_number,
                    line);
    }

    // sources of violation
    line_number++;
    int source_line_number = line_number;
    std::getline(report, line);
    std::string sources;
    boost::smatch sources_match;
    if (boost::regex_match(line, sources_match, srcs)) {
      sources = sources_match[1].str();
    } else {
      logger->error(utl::ODB,
                    101,
                    "Unable to parse line as violation source (line: {}): {}",
                    line_number,
                    line);
    }

    line_number++;
    std::getline(report, line);
    std::string comment_information;

    // comment information (optional)
    if (std::regex_match(line, base_match, comment_line)) {
      comment_information = base_match[2].str();
      line_number++;
      std::getline(report, line);
    }

    // bounding box and layer
    if (!std::regex_match(line, base_match, bbox_layer)) {
      logger->error(utl::ODB,
                    291,
                    "Unable to parse line as violation location (line: {}): {}",
                    line_number,
                    line);
    }

    std::string bbox = base_match[1].str();
    odb::dbTechLayer* layer = tech->findLayer(base_match[2].str().c_str());
    if (layer == nullptr && base_match[2].str() != "-") {
      logger->warn(utl::ODB,
                   224,
                   "Unable to find tech layer (line: {}): {}",
                   line_number,
                   base_match[2].str());
    }

    odb::Rect rect;
    if (std::regex_match(bbox, base_match, bbox_corners)) {
      try {
        rect.set_xlo(std::stod(base_match[1].str())
                     * block->getDbUnitsPerMicron());
        rect.set_ylo(std::stod(base_match[2].str())
                     * block->getDbUnitsPerMicron());
        rect.set_xhi(std::stod(base_match[3].str())
                     * block->getDbUnitsPerMicron());
        rect.set_yhi(std::stod(base_match[4].str())
                     * block->getDbUnitsPerMicron());
      } catch (std::invalid_argument&) {
        logger->error(utl::ODB,
                      225,
                      "Unable to parse bounding box (line: {}): {}",
                      line_number,
                      bbox);
      } catch (std::out_of_range&) {
        logger->error(utl::ODB,
                      226,
                      "Unable to parse bounding box (line: {}): {}",
                      line_number,
                      bbox);
      }
    } else {
      logger->error(utl::ODB,
                    227,
                    "Unable to parse bounding box (line: {}): {}",
                    line_number,
                    bbox);
    }

    std::stringstream srcs_stream(sources);
    std::string single_source;
    std::string comment;

    dbMarkerCategory* category
        = odb::dbMarkerCategory::createOrGet(marker_category, type.c_str());

    dbMarker* marker = dbMarker::create(category);
    if (marker == nullptr) {
      continue;
    }

    marker->setTechLayer(layer);
    marker->setLineNumber(violation_line_number);
    marker->addShape(rect);

    // split sources list
    while (getline(srcs_stream, single_source, ' ')) {
      if (single_source.empty()) {
        continue;
      }

      auto ident = single_source.find(':');
      std::string item_type = single_source.substr(0, ident);
      std::string item_name = single_source.substr(ident + 1);

      bool src_found = false;
      if (item_type == "net") {
        dbNet* net = block->findNet(item_name.c_str());
        if (net != nullptr) {
          marker->addSource(net);
          src_found = true;
        } else {
          logger->warn(utl::ODB,
                       234,
                       "Unable to find net (line: {}): {}",
                       source_line_number,
                       item_name);
        }
      } else if (item_type == "inst") {
        dbInst* inst = block->findInst(item_name.c_str());
        if (inst != nullptr) {
          marker->addSource(inst);
          src_found = true;
        } else {
          logger->warn(utl::ODB,
                       235,
                       "Unable to find instance (line: {}): {}",
                       source_line_number,
                       item_name);
        }
      } else if (item_type == "iterm") {
        dbITerm* iterm = block->findITerm(item_name.c_str());
        if (iterm != nullptr) {
          marker->addSource(iterm);
          src_found = true;
        } else {
          logger->warn(utl::ODB,
                       236,
                       "Unable to find iterm (line: {}): {}",
                       source_line_number,
                       item_name);
        }
      } else if (item_type == "bterm") {
        dbBTerm* bterm = block->findBTerm(item_name.c_str());
        if (bterm != nullptr) {
          marker->addSource(bterm);
          src_found = true;
        } else {
          logger->warn(utl::ODB,
                       237,
                       "Unable to find bterm (line: {}): {}",
                       source_line_number,
                       item_name);
        }
      } else if (item_type == "obstruction") {
        src_found = marker->addObstructionFromBlock(block);
      } else {
        logger->warn(utl::ODB,
                     233,
                     "Unknown source type (line: {}): {}",
                     source_line_number,
                     item_type);
      }

      if (!src_found && !item_name.empty()) {
        comment += single_source + " ";
      }
    }

    comment += comment_information;
    marker->setComment(comment);
  }

  return marker_category;
}

std::set<dbMarker*> dbMarkerCategory::getAllMarkers() const
{
  std::set<dbMarker*> markers;

  for (dbMarkerCategory* category : getMarkerCategorys()) {
    const std::set<dbMarker*> category_markers = category->getAllMarkers();
    markers.insert(category_markers.begin(), category_markers.end());
  }

  for (dbMarker* marker : getMarkers()) {
    markers.insert(marker);
  }

  return markers;
}

dbMarkerCategory* dbMarkerCategory::create(dbBlock* block, const char* name)
{
  _dbBlock* parent = (_dbBlock*) block;

  if (parent->_marker_category_hash.hasMember(name)) {
    return nullptr;
  }

  _dbMarkerCategory* _category = parent->_marker_categories_tbl->create();

  _category->_name = strdup(name);
  ZALLOCATED(_category->_name);

  parent->_marker_category_hash.insert(_category);

  std::list<dbBlockCallBackObj*>::iterator cbitr;
  for (cbitr = parent->_callbacks.begin(); cbitr != parent->_callbacks.end();
       ++cbitr) {
    (**cbitr)().inDbMarkerCategoryCreate((dbMarkerCategory*) _category);
  }

  return (dbMarkerCategory*) _category;
}

dbMarkerCategory* dbMarkerCategory::createOrGet(dbBlock* block,
                                                const char* name)
{
  _dbBlock* parent = (_dbBlock*) block;

  if (parent->_marker_category_hash.hasMember(name)) {
    return (dbMarkerCategory*) parent->_marker_category_hash.find(name);
  }

  return create(block, name);
}

dbMarkerCategory* dbMarkerCategory::createOrReplace(dbBlock* block,
                                                    const char* name)
{
  _dbBlock* parent = (_dbBlock*) block;

  if (parent->_marker_category_hash.hasMember(name)) {
    destroy((dbMarkerCategory*) parent->_marker_category_hash.find(name));
  }

  return create(block, name);
}

dbMarkerCategory* dbMarkerCategory::create(dbMarkerCategory* category,
                                           const char* name)
{
  _dbMarkerCategory* parent = (_dbMarkerCategory*) category;

  if (parent->categories_hash_.hasMember(name)) {
    return nullptr;
  }

  _dbMarkerCategory* _category = parent->categories_tbl_->create();

  _category->_name = strdup(name);
  ZALLOCATED(_category->_name);

  parent->categories_hash_.insert(_category);

  _dbBlock* block = parent->getBlock();
  std::list<dbBlockCallBackObj*>::iterator cbitr;
  for (cbitr = block->_callbacks.begin(); cbitr != block->_callbacks.end();
       ++cbitr) {
    (**cbitr)().inDbMarkerCategoryCreate((dbMarkerCategory*) _category);
  }

  return (dbMarkerCategory*) _category;
}

dbMarkerCategory* dbMarkerCategory::createOrGet(dbMarkerCategory* category,
                                                const char* name)
{
  _dbMarkerCategory* parent = (_dbMarkerCategory*) category;

  if (parent->categories_hash_.hasMember(name)) {
    return (dbMarkerCategory*) parent->categories_hash_.find(name);
  }

  return create(category, name);
}

dbMarkerCategory* dbMarkerCategory::createOrReplace(dbMarkerCategory* category,
                                                    const char* name)
{
  _dbMarkerCategory* parent = (_dbMarkerCategory*) category;

  if (parent->categories_hash_.hasMember(name)) {
    destroy((dbMarkerCategory*) parent->categories_hash_.find(name));
  }

  return create(category, name);
}

void dbMarkerCategory::destroy(dbMarkerCategory* category)
{
  _dbMarkerCategory* _category = (_dbMarkerCategory*) category;

  if (_category->isTopCategory()) {
    _dbBlock* block = (_dbBlock*) _category->getOwner();

    std::list<dbBlockCallBackObj*>::iterator cbitr;
    for (cbitr = block->_callbacks.begin(); cbitr != block->_callbacks.end();
         ++cbitr) {
      (**cbitr)().inDbMarkerCategoryDestroy(category);
    }

    block->_marker_category_hash.remove(_category);
    block->_marker_categories_tbl->destroy(_category);
  } else {
    _dbMarkerCategory* parent = (_dbMarkerCategory*) _category->getOwner();

    _dbBlock* block = parent->getBlock();
    std::list<dbBlockCallBackObj*>::iterator cbitr;
    for (cbitr = block->_callbacks.begin(); cbitr != block->_callbacks.end();
         ++cbitr) {
      (**cbitr)().inDbMarkerCategoryDestroy(category);
    }

    parent->categories_hash_.remove(_category);
    parent->categories_tbl_->destroy(_category);
  }
}

// User Code End dbMarkerCategoryPublicMethods
}  // namespace odb
   // Generator Code End Cpp