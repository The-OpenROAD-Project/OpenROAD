///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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
#include "dbLevelShifter.h"

#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbMaster.h"
#include "dbNet.h"
#include "dbPowerDomain.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbLevelShifter>;

bool _dbLevelShifter::operator==(const _dbLevelShifter& rhs) const
{
  if (_name != rhs._name) {
    return false;
  }
  if (_next_entry != rhs._next_entry) {
    return false;
  }
  if (_domain != rhs._domain) {
    return false;
  }
  if (_source != rhs._source) {
    return false;
  }
  if (_sink != rhs._sink) {
    return false;
  }
  if (_use_functional_equivalence != rhs._use_functional_equivalence) {
    return false;
  }
  if (_applies_to != rhs._applies_to) {
    return false;
  }
  if (_applies_to_boundary != rhs._applies_to_boundary) {
    return false;
  }
  if (_rule != rhs._rule) {
    return false;
  }
  if (_threshold != rhs._threshold) {
    return false;
  }
  if (_no_shift != rhs._no_shift) {
    return false;
  }
  if (_force_shift != rhs._force_shift) {
    return false;
  }
  if (_location != rhs._location) {
    return false;
  }
  if (_input_supply != rhs._input_supply) {
    return false;
  }
  if (_output_supply != rhs._output_supply) {
    return false;
  }
  if (_internal_supply != rhs._internal_supply) {
    return false;
  }
  if (_name_prefix != rhs._name_prefix) {
    return false;
  }
  if (_name_suffix != rhs._name_suffix) {
    return false;
  }
  if (_cell_name != rhs._cell_name) {
    return false;
  }
  if (_cell_input != rhs._cell_input) {
    return false;
  }
  if (_cell_output != rhs._cell_output) {
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
  _name = nullptr;
  _use_functional_equivalence = false;
  _threshold = 0;
  _no_shift = false;
  _force_shift = false;
}

dbIStream& operator>>(dbIStream& stream, _dbLevelShifter& obj)
{
  stream >> obj._name;
  stream >> obj._next_entry;
  stream >> obj._domain;
  stream >> obj._elements;
  stream >> obj._exclude_elements;
  stream >> obj._source;
  stream >> obj._sink;
  stream >> obj._use_functional_equivalence;
  stream >> obj._applies_to;
  stream >> obj._applies_to_boundary;
  stream >> obj._rule;
  stream >> obj._threshold;
  stream >> obj._no_shift;
  stream >> obj._force_shift;
  stream >> obj._location;
  stream >> obj._input_supply;
  stream >> obj._output_supply;
  stream >> obj._internal_supply;
  stream >> obj._name_prefix;
  stream >> obj._name_suffix;
  stream >> obj._instances;
  // User Code Begin >>
  if (stream.getDatabase()->isSchema(db_schema_level_shifter_cell)) {
    stream >> obj._cell_name;
    stream >> obj._cell_input;
    stream >> obj._cell_output;
  }
  // User Code End >>
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbLevelShifter& obj)
{
  stream << obj._name;
  stream << obj._next_entry;
  stream << obj._domain;
  stream << obj._elements;
  stream << obj._exclude_elements;
  stream << obj._source;
  stream << obj._sink;
  stream << obj._use_functional_equivalence;
  stream << obj._applies_to;
  stream << obj._applies_to_boundary;
  stream << obj._rule;
  stream << obj._threshold;
  stream << obj._no_shift;
  stream << obj._force_shift;
  stream << obj._location;
  stream << obj._input_supply;
  stream << obj._output_supply;
  stream << obj._internal_supply;
  stream << obj._name_prefix;
  stream << obj._name_suffix;
  stream << obj._instances;
  // User Code Begin <<
  stream << obj._cell_name;
  stream << obj._cell_input;
  stream << obj._cell_output;
  // User Code End <<
  return stream;
}

void _dbLevelShifter::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children_["name"].add(_name);
  info.children_["_elements"].add(_elements);
  info.children_["_exclude_elements"].add(_exclude_elements);
  info.children_["_source"].add(_source);
  info.children_["_sink"].add(_sink);
  info.children_["_applies_to"].add(_applies_to);
  info.children_["_applies_to_boundary"].add(_applies_to_boundary);
  info.children_["_rule"].add(_rule);
  info.children_["_location"].add(_location);
  info.children_["_input_supply"].add(_input_supply);
  info.children_["_output_supply"].add(_output_supply);
  info.children_["_internal_supply"].add(_internal_supply);
  info.children_["_name_prefix"].add(_name_prefix);
  info.children_["_name_suffix"].add(_name_suffix);
  info.children_["_instances"].add(_instances);
  info.children_["_cell_name"].add(_cell_name);
  info.children_["_cell_input"].add(_cell_input);
  info.children_["_cell_output"].add(_cell_output);
  // User Code End collectMemInfo
}

_dbLevelShifter::~_dbLevelShifter()
{
  if (_name) {
    free((void*) _name);
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
  return obj->_name;
}

dbPowerDomain* dbLevelShifter::getDomain() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  if (obj->_domain == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbPowerDomain*) par->_powerdomain_tbl->getPtr(obj->_domain);
}

void dbLevelShifter::setSource(const std::string& source)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->_source = source;
}

std::string dbLevelShifter::getSource() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->_source;
}

void dbLevelShifter::setSink(const std::string& sink)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->_sink = sink;
}

std::string dbLevelShifter::getSink() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->_sink;
}

void dbLevelShifter::setUseFunctionalEquivalence(
    bool use_functional_equivalence)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->_use_functional_equivalence = use_functional_equivalence;
}

bool dbLevelShifter::isUseFunctionalEquivalence() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->_use_functional_equivalence;
}

void dbLevelShifter::setAppliesTo(const std::string& applies_to)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->_applies_to = applies_to;
}

std::string dbLevelShifter::getAppliesTo() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->_applies_to;
}

void dbLevelShifter::setAppliesToBoundary(
    const std::string& applies_to_boundary)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->_applies_to_boundary = applies_to_boundary;
}

std::string dbLevelShifter::getAppliesToBoundary() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->_applies_to_boundary;
}

void dbLevelShifter::setRule(const std::string& rule)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->_rule = rule;
}

std::string dbLevelShifter::getRule() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->_rule;
}

void dbLevelShifter::setThreshold(float threshold)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->_threshold = threshold;
}

float dbLevelShifter::getThreshold() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->_threshold;
}

void dbLevelShifter::setNoShift(bool no_shift)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->_no_shift = no_shift;
}

bool dbLevelShifter::isNoShift() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->_no_shift;
}

void dbLevelShifter::setForceShift(bool force_shift)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->_force_shift = force_shift;
}

bool dbLevelShifter::isForceShift() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->_force_shift;
}

void dbLevelShifter::setLocation(const std::string& location)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->_location = location;
}

std::string dbLevelShifter::getLocation() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->_location;
}

void dbLevelShifter::setInputSupply(const std::string& input_supply)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->_input_supply = input_supply;
}

std::string dbLevelShifter::getInputSupply() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->_input_supply;
}

void dbLevelShifter::setOutputSupply(const std::string& output_supply)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->_output_supply = output_supply;
}

std::string dbLevelShifter::getOutputSupply() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->_output_supply;
}

void dbLevelShifter::setInternalSupply(const std::string& internal_supply)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->_internal_supply = internal_supply;
}

std::string dbLevelShifter::getInternalSupply() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->_internal_supply;
}

void dbLevelShifter::setNamePrefix(const std::string& name_prefix)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->_name_prefix = name_prefix;
}

std::string dbLevelShifter::getNamePrefix() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->_name_prefix;
}

void dbLevelShifter::setNameSuffix(const std::string& name_suffix)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->_name_suffix = name_suffix;
}

std::string dbLevelShifter::getNameSuffix() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->_name_suffix;
}

void dbLevelShifter::setCellName(const std::string& cell_name)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->_cell_name = cell_name;
}

std::string dbLevelShifter::getCellName() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->_cell_name;
}

void dbLevelShifter::setCellInput(const std::string& cell_input)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->_cell_input = cell_input;
}

std::string dbLevelShifter::getCellInput() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->_cell_input;
}

void dbLevelShifter::setCellOutput(const std::string& cell_output)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;

  obj->_cell_output = cell_output;
}

std::string dbLevelShifter::getCellOutput() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->_cell_output;
}

// User Code Begin dbLevelShifterPublicMethods

dbLevelShifter* dbLevelShifter::create(dbBlock* block,
                                       const char* name,
                                       dbPowerDomain* domain)
{
  _dbBlock* _block = (_dbBlock*) block;
  if (_block->_levelshifter_hash.hasMember(name)) {
    return nullptr;
  }

  if (domain == nullptr) {
    return nullptr;
  }

  _dbLevelShifter* shifter = _block->_levelshifter_tbl->create();
  shifter->_name = strdup(name);
  ZALLOCATED(shifter->_name);

  shifter->_domain = domain->getImpl()->getOID();

  _block->_levelshifter_hash.insert(shifter);

  domain->addLevelShifter((dbLevelShifter*) shifter);

  return (dbLevelShifter*) shifter;
}

void dbLevelShifter::destroy(dbLevelShifter* shifter)
{
  _dbLevelShifter* _shifter = (_dbLevelShifter*) shifter;
  _dbBlock* block = (_dbBlock*) _shifter->getOwner();

  if (block->_levelshifter_hash.hasMember(_shifter->_name)) {
    block->_levelshifter_hash.remove(_shifter);
  }

  block->_levelshifter_tbl->destroy(_shifter);
}

void dbLevelShifter::addElement(const std::string& element)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  obj->_elements.push_back(element);
}
void dbLevelShifter::addExcludeElement(const std::string& element)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  obj->_exclude_elements.push_back(element);
}
void dbLevelShifter::addInstance(const std::string& instance,
                                 const std::string& port)
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  obj->_instances.push_back(std::make_pair(instance, port));
}

std::vector<std::string> dbLevelShifter::getElements() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->_elements;
}
std::vector<std::string> dbLevelShifter::getExcludeElements() const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->_exclude_elements;
}
std::vector<std::pair<std::string, std::string>> dbLevelShifter::getInstances()
    const
{
  _dbLevelShifter* obj = (_dbLevelShifter*) this;
  return obj->_instances;
}

// User Code End dbLevelShifterPublicMethods
}  // namespace odb
   // Generator Code End Cpp
