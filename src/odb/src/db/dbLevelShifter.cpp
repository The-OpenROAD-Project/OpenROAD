// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbLevelShifter.h"

#include <cstdlib>
#include <string>

#include "dbBlock.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbMaster.h"
#include "dbNet.h"
#include "dbPowerDomain.h"
#include "dbTable.h"
#include "odb/db.h"
// User Code Begin Includes
#include "dbCommon.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbLevelShifter>;

bool _dbLevelShifter::operator==(const _dbLevelShifter& rhs) const
{
  if (name_ != rhs.name_) {
    return false;
  }
  if (next_entry_ != rhs.next_entry_) {
    return false;
  }
  if (domain_ != rhs.domain_) {
    return false;
  }
  if (source_ != rhs.source_) {
    return false;
  }
  if (sink_ != rhs.sink_) {
    return false;
  }
  if (use_functional_equivalence_ != rhs.use_functional_equivalence_) {
    return false;
  }
  if (applies_to_ != rhs.applies_to_) {
    return false;
  }
  if (applies_to_boundary_ != rhs.applies_to_boundary_) {
    return false;
  }
  if (rule_ != rhs.rule_) {
    return false;
  }
  if (threshold_ != rhs.threshold_) {
    return false;
  }
  if (no_shift_ != rhs.no_shift_) {
    return false;
  }
  if (force_shift_ != rhs.force_shift_) {
    return false;
  }
  if (location_ != rhs.location_) {
    return false;
  }
  if (input_supply_ != rhs.input_supply_) {
    return false;
  }
  if (output_supply_ != rhs.output_supply_) {
    return false;
  }
  if (internal_supply_ != rhs.internal_supply_) {
    return false;
  }
  if (name_prefix_ != rhs.name_prefix_) {
    return false;
  }
  if (name_suffix_ != rhs.name_suffix_) {
    return false;
  }
  if (cell_name_ != rhs.cell_name_) {
    return false;
  }
  if (cell_input_ != rhs.cell_input_) {
    return false;
  }
  if (cell_output_ != rhs.cell_output_) {
    return false;
  }

  return true;
}

bool _dbLevelShifter::operator<(const _dbLevelShifter& rhs) const
{
  return true;
}

_dbLevelShifter::_dbLevelShifter(_dbDatabase* db)
{
  name_ = nullptr;
  use_functional_equivalence_ = false;
  threshold_ = 0;
  no_shift_ = false;
  force_shift_ = false;
}

dbIStream& operator>>(dbIStream& stream, _dbLevelShifter& obj)
{
  stream >> obj.name_;
  stream >> obj.next_entry_;
  stream >> obj.domain_;
  stream >> obj.elements_;
  stream >> obj.exclude_elements_;
  stream >> obj.source_;
  stream >> obj.sink_;
  stream >> obj.use_functional_equivalence_;
  stream >> obj.applies_to_;
  stream >> obj.applies_to_boundary_;
  stream >> obj.rule_;
  stream >> obj.threshold_;
  stream >> obj.no_shift_;
  stream >> obj.force_shift_;
  stream >> obj.location_;
  stream >> obj.input_supply_;
  stream >> obj.output_supply_;
  stream >> obj.internal_supply_;
  stream >> obj.name_prefix_;
  stream >> obj.name_suffix_;
  stream >> obj.instances_;
  // User Code Begin >>
  if (stream.getDatabase()->isSchema(kSchemaLevelShifterCell)) {
    stream >> obj.cell_name_;
    stream >> obj.cell_input_;
    stream >> obj.cell_output_;
  }
  // User Code End >>
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbLevelShifter& obj)
{
  stream << obj.name_;
  stream << obj.next_entry_;
  stream << obj.domain_;
  stream << obj.elements_;
  stream << obj.exclude_elements_;
  stream << obj.source_;
  stream << obj.sink_;
  stream << obj.use_functional_equivalence_;
  stream << obj.applies_to_;
  stream << obj.applies_to_boundary_;
  stream << obj.rule_;
  stream << obj.threshold_;
  stream << obj.no_shift_;
  stream << obj.force_shift_;
  stream << obj.location_;
  stream << obj.input_supply_;
  stream << obj.output_supply_;
  stream << obj.internal_supply_;
  stream << obj.name_prefix_;
  stream << obj.name_suffix_;
  stream << obj.instances_;
  // User Code Begin <<
  stream << obj.cell_name_;
  stream << obj.cell_input_;
  stream << obj.cell_output_;
  // User Code End <<
  return stream;
}

void _dbLevelShifter::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children["name"].add(name_);
  info.children["_elements"].add(elements_);
  info.children["_exclude_elements"].add(exclude_elements_);
  info.children["_source"].add(source_);
  info.children["_sink"].add(sink_);
  info.children["_applies_to"].add(applies_to_);
  info.children["_applies_to_boundary"].add(applies_to_boundary_);
  info.children["_rule"].add(rule_);
  info.children["_location"].add(location_);
  info.children["_input_supply"].add(input_supply_);
  info.children["_output_supply"].add(output_supply_);
  info.children["_internal_supply"].add(internal_supply_);
  info.children["_name_prefix"].add(name_prefix_);
  info.children["_name_suffix"].add(name_suffix_);
  info.children["_instances"].add(instances_);
  info.children["_cell_name"].add(cell_name_);
  info.children["_cell_input"].add(cell_input_);
  info.children["_cell_output"].add(cell_output_);
  // User Code End collectMemInfo
}

_dbLevelShifter::~_dbLevelShifter()
{
  if (name_) {
    free((void*) name_);
  }
}

////////////////////////////////////////////////////////////////////
//
// dbLevelShifter - Methods
//
////////////////////////////////////////////////////////////////////

const char* dbLevelShifter::getName() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->name_;
}

dbPowerDomain* dbLevelShifter::getDomain() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  if (obj->domain_ == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbPowerDomain*) par->powerdomain_tbl_->getPtr(obj->domain_);
}

void dbLevelShifter::setSource(const std::string& source)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->source_ = source;
}

std::string dbLevelShifter::getSource() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->source_;
}

void dbLevelShifter::setSink(const std::string& sink)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->sink_ = sink;
}

std::string dbLevelShifter::getSink() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->sink_;
}

void dbLevelShifter::setUseFunctionalEquivalence(
    bool use_functional_equivalence)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->use_functional_equivalence_ = use_functional_equivalence;
}

bool dbLevelShifter::isUseFunctionalEquivalence() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->use_functional_equivalence_;
}

void dbLevelShifter::setAppliesTo(const std::string& applies_to)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->applies_to_ = applies_to;
}

std::string dbLevelShifter::getAppliesTo() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->applies_to_;
}

void dbLevelShifter::setAppliesToBoundary(
    const std::string& applies_to_boundary)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->applies_to_boundary_ = applies_to_boundary;
}

std::string dbLevelShifter::getAppliesToBoundary() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->applies_to_boundary_;
}

void dbLevelShifter::setRule(const std::string& rule)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->rule_ = rule;
}

std::string dbLevelShifter::getRule() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->rule_;
}

void dbLevelShifter::setThreshold(float threshold)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->threshold_ = threshold;
}

float dbLevelShifter::getThreshold() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->threshold_;
}

void dbLevelShifter::setNoShift(bool no_shift)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->no_shift_ = no_shift;
}

bool dbLevelShifter::isNoShift() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->no_shift_;
}

void dbLevelShifter::setForceShift(bool force_shift)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->force_shift_ = force_shift;
}

bool dbLevelShifter::isForceShift() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->force_shift_;
}

void dbLevelShifter::setLocation(const std::string& location)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->location_ = location;
}

std::string dbLevelShifter::getLocation() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->location_;
}

void dbLevelShifter::setInputSupply(const std::string& input_supply)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->input_supply_ = input_supply;
}

std::string dbLevelShifter::getInputSupply() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->input_supply_;
}

void dbLevelShifter::setOutputSupply(const std::string& output_supply)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->output_supply_ = output_supply;
}

std::string dbLevelShifter::getOutputSupply() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->output_supply_;
}

void dbLevelShifter::setInternalSupply(const std::string& internal_supply)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->internal_supply_ = internal_supply;
}

std::string dbLevelShifter::getInternalSupply() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->internal_supply_;
}

void dbLevelShifter::setNamePrefix(const std::string& name_prefix)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->name_prefix_ = name_prefix;
}

std::string dbLevelShifter::getNamePrefix() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->name_prefix_;
}

void dbLevelShifter::setNameSuffix(const std::string& name_suffix)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->name_suffix_ = name_suffix;
}

std::string dbLevelShifter::getNameSuffix() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->name_suffix_;
}

void dbLevelShifter::setCellName(const std::string& cell_name)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->cell_name_ = cell_name;
}

std::string dbLevelShifter::getCellName() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->cell_name_;
}

void dbLevelShifter::setCellInput(const std::string& cell_input)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->cell_input_ = cell_input;
}

std::string dbLevelShifter::getCellInput() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->cell_input_;
}

void dbLevelShifter::setCellOutput(const std::string& cell_output)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->cell_output_ = cell_output;
}

std::string dbLevelShifter::getCellOutput() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->cell_output_;
}

// User Code Begin dbLevelShifterPublicMethods

dbLevelShifter* dbLevelShifter::create(dbBlock* block,
                                       const char* name,
                                       dbPowerDomain* domain)
{
  _dbBlock* _block = (_dbBlock*) block;
  if (_block->levelshifter_hash_.hasMember(name)) {
    return nullptr;
  }

  if (domain == nullptr) {
    return nullptr;
  }

  _dbLevelShifter* shifter = _block->levelshifter_tbl_->create();
  shifter->name_ = safe_strdup(name);

  shifter->domain_ = domain->getImpl()->getOID();

  _block->levelshifter_hash_.insert(shifter);

  domain->addLevelShifter((dbLevelShifter*) shifter);

  return (dbLevelShifter*) shifter;
}

void dbLevelShifter::destroy(dbLevelShifter* shifter)
{
  _dbLevelShifter* _shifter = (_dbLevelShifter*) shifter;
  _dbBlock* block = (_dbBlock*) _shifter->getOwner();

  if (block->levelshifter_hash_.hasMember(_shifter->name_)) {
    block->levelshifter_hash_.remove(_shifter);
  }

  block->levelshifter_tbl_->destroy(_shifter);
}

void dbLevelShifter::addElement(const std::string& element)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  obj->elements_.push_back(element);
}
void dbLevelShifter::addExcludeElement(const std::string& element)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  obj->exclude_elements_.push_back(element);
}
void dbLevelShifter::addInstance(const std::string& instance,
                                 const std::string& port)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  obj->instances_.push_back(std::make_pair(instance, port));
}

std::vector<std::string> dbLevelShifter::getElements() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->elements_;
}
std::vector<std::string> dbLevelShifter::getExcludeElements() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->exclude_elements_;
}
std::vector<std::pair<std::string, std::string>> dbLevelShifter::getInstances()
    const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->instances_;
}

// User Code End dbLevelShifterPublicMethods
}  // namespace odb
   // Generator Code End Cpp
