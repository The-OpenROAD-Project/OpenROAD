// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "cut/abc_library_factory.h"

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()

#include <cmath>
#include <cstring>
#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

#include "db_sta/dbSta.hh"
#include "misc/util/abc_global.h"
#include "misc/vec/vecFlt.h"
#include "misc/vec/vecInt.h"
#include "misc/vec/vecPtr.h"
#include "misc/vec/vecWrd.h"
#include "rsz/Resizer.hh"
#include "sta/LibertyClass.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingModel.hh"
#include "utl/Logger.h"
// Poor include definitions in ABC
// clang-format off
#include "misc/st/st.h"
#include "map/mio/mio.h"
#include "misc/util/utilNam.h"
#include "map/scl/sclCon.h"
// clang-format on
#include "map/scl/sclLib.h"
#include "sta/Corner.hh"
#include "sta/FuncExpr.hh"
#include "sta/LeakagePower.hh"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "sta/Sta.hh"
#include "sta/TableModel.hh"
#include "sta/Units.hh"
#include "utl/SuppressStdout.h"
#include "utl/deleter.h"

namespace abc {
// Forward declare instead of including to avoid warnings from ABC
void Abc_Start();
void Abc_Stop();
}  // namespace abc

namespace cut {

AbcLibrary::AbcLibrary(utl::UniquePtrWithDeleter<abc::SC_Lib> abc_library,
                       utl::Logger* logger)
    : abc_library_(std::move(abc_library)), logger_(logger)
{
  mio_library_ = utl::UniquePtrWithDeleter<abc::Mio_Library_t>(
      abc::Abc_SclDeriveGenlibSimple(abc_library_.get()),
      [](abc::Mio_Library_t* lib) { abc::Mio_LibraryDelete(lib); });
}

abc::Mio_Library_t* AbcLibrary::mio_library()
{
  if (mio_library_ == nullptr) {
    utl::SuppressStdout nostdout(logger_);
    mio_library_ = utl::UniquePtrWithDeleter<abc::Mio_Library_t>(
        abc::Abc_SclDeriveGenlibSimple(abc_library_.get()),
        [](abc::Mio_Library_t* lib) { abc::Mio_LibraryDelete(lib); });
  }
  return mio_library_.release();
}

static bool IsCombinational(sta::LibertyCell* cell)
{
  if (!cell) {
    return false;
  }
  return (!cell->isClockGate() && !cell->isPad() && !cell->isMacro()
          && !cell->hasSequentials() && !cell->isLevelShifter()
          && !cell->isIsolationCell() && !cell->isMemory());
}

static int CountOutputPins(sta::LibertyCell* cell)
{
  sta::LibertyCellPortIterator cell_port_iterator(cell);
  int output_count = 0;
  while (cell_port_iterator.hasNext()) {
    sta::LibertyPort* port = cell_port_iterator.next();
    if (port->direction()->isOutput()) {
      output_count++;
    }
  }

  return output_count;
}

static bool HasNonInputOutputPorts(sta::LibertyCell* cell)
{
  sta::LibertyCellPortIterator cell_port_iterator(cell);
  while (cell_port_iterator.hasNext()) {
    sta::LibertyPort* port = cell_port_iterator.next();
    if (port->direction()->isOutput()) {
      continue;
    }
    if (port->direction()->isInput()) {
      continue;
    }
    if (port->isPwrGnd()) {
      continue;
    }

    return true;
  }

  return false;
}

static bool isCompatibleWithAbc(sta::LibertyCell* cell, rsz::Resizer* resizer)
{
  if (resizer != nullptr && resizer->dontUse(cell)) {
    return false;
  }

  if (!IsCombinational(cell)) {
    return false;
  }

  // ABC requires at least one output pin.
  if (CountOutputPins(cell) == 0) {
    return false;
  }

  if (HasNonInputOutputPorts(cell)) {
    return false;
  }

  return true;
}

void AbcLibraryFactory::AbcPopulateAbcSurfaceFromSta(
    abc::SC_Surface* abc_table,
    const sta::TableModel* model,
    sta::Units* units)
{
  sta::Unit* time_unit = units->timeUnit();
  sta::Unit* capacitance_unit = units->capacitanceUnit();

  // 2 axis tables are expected in Liberty timing constructs
  if (model->order() != 2) {
    logger_->error(utl::CUT,
                   14,
                   "sta::TableModel order not equal to 2, order: {}",
                   model->order());
  }

  const sta::TableAxis* axis_1 = model->axis1();
  sta::FloatSeq* axis1_values = axis_1->values();
  if (!axis1_values) {
    logger_->error(utl::CUT, 15, "axis 1 null cannot create ABC table");
  }

  for (float axis_value : *axis1_values) {
    double adjusted_value = time_unit->staToUser(axis_value);
    abc::Vec_FltPush(&abc_table->vIndex0, adjusted_value);
    abc::Vec_IntPush(&abc_table->vIndex0I, abc::Scl_Flt2Int(adjusted_value));
  }

  const sta::TableAxis* axis_2 = model->axis2();
  sta::FloatSeq* axis2_values = axis_2->values();
  if (!axis2_values) {
    logger_->error(utl::CUT, 16, "axis 2 null cannot create ABC table");
  }

  for (float axis_value : *axis2_values) {
    double adjusted_value = capacitance_unit->staToUser(axis_value);
    abc::Vec_FltPush(&abc_table->vIndex1, adjusted_value);
    abc::Vec_IntPush(&abc_table->vIndex1I, abc::Scl_Flt2Int(adjusted_value));
  }

  // Vec<Vec<float> > -- 'vData[i0][i1]' gives value at '(index0[i0],
  // index1[i1])' Building the 2D table from STA to ABC's data structure.
  for (size_t i = 0; i < axis1_values->size(); i++) {
    abc::Vec_Flt_t* axis_1_abc_vec = abc::Vec_FltAlloc(axis1_values->size());
    abc::Vec_Int_t* axis_1_abc_int_vec
        = abc::Vec_IntAlloc(axis1_values->size());
    abc::Vec_PtrPush(&abc_table->vData, axis_1_abc_vec);
    abc::Vec_PtrPush(&abc_table->vDataI, axis_1_abc_int_vec);

    for (size_t j = 0; j < axis2_values->size(); j++) {
      float value = time_unit->staToUser(model->value(i, j, 0));
      abc::Vec_FltPush(axis_1_abc_vec, value);
      abc::Vec_IntPush(axis_1_abc_int_vec, abc::Scl_Flt2Int(value));
    }
  }
}

std::vector<abc::SC_Pin*> AbcLibraryFactory::CreateAbcInputPins(
    sta::LibertyCell* cell)
{
  sta::Units* units = cell->libertyLibrary()->units();
  sta::Unit* capacitance_unit = units->capacitanceUnit();

  std::vector<abc::SC_Pin*> result;
  sta::LibertyCellPortIterator cell_port_iterator(cell);
  while (cell_port_iterator.hasNext()) {
    sta::LibertyPort* cell_port = cell_port_iterator.next();
    if (!cell_port->direction()->isInput()) {
      continue;
    }

    abc::SC_Pin* input_pin = abc::Abc_SclPinAlloc();
    input_pin->dir = abc::sc_dir_Input;
    input_pin->pName = strdup(cell_port->name());
    input_pin->rise_cap = capacitance_unit->staToUser(
        cell_port->capacitance(sta::RiseFall::rise(), sta::MinMax::max()));
    input_pin->fall_cap = capacitance_unit->staToUser(
        cell_port->capacitance(sta::RiseFall::fall(), sta::MinMax::max()));

    result.push_back(input_pin);
  }

  return result;
}

std::vector<abc::SC_Pin*> AbcLibraryFactory::CreateAbcOutputPins(
    sta::LibertyCell* cell,
    const std::vector<std::string>& input_names)
{
  sta::Units* units = cell->libertyLibrary()->units();
  sta::Unit* time_unit = units->timeUnit();
  sta::Unit* cap_unit = units->capacitanceUnit();

  std::vector<abc::SC_Pin*> result;
  sta::LibertyCellPortIterator cell_port_iterator(cell);
  while (cell_port_iterator.hasNext()) {
    sta::LibertyPort* cell_port = cell_port_iterator.next();
    if (!cell_port->direction()->isOutput()) {
      continue;
    }

    abc::SC_Pin* output_pin = abc::Abc_SclPinAlloc();
    output_pin->dir = abc::sc_dir_Output;
    output_pin->pName = strdup(cell_port->name());

    float max_output_capacitance = 0;
    bool exists = false;
    cell_port->capacitanceLimit(
        sta::MinMax::max(), max_output_capacitance, exists);
    if (exists) {
      output_pin->max_out_cap = cap_unit->staToUser(max_output_capacitance);
    }

    float max_output_slew = 0;
    cell_port->slewLimit(sta::MinMax::max(), max_output_slew, exists);
    if (exists) {
      output_pin->max_out_slew = time_unit->staToUser(max_output_slew);
    }

    if (cell_port->function() == nullptr) {
      logger_->error(utl::CUT,
                     49,
                     "cell port function is null for cell {}:{}",
                     cell->name(),
                     cell_port->name());
    }
    output_pin->func_text = strdup(cell_port->function()->to_string().c_str());

    // Get list of input ports
    abc::Vec_Ptr_t* input_names_abc = abc::Vec_PtrAlloc(input_names.size());
    for (const std::string& port_name : input_names) {
      abc::Vec_PtrPush(input_names_abc, const_cast<char*>(port_name.data()));
    }

    // Set standard cell function
    abc::Vec_Wrd_t* vFunc;
    abc::Vec_WrdErase(&output_pin->vFunc);
    vFunc = abc::Mio_ParseFormulaTruth(
        output_pin->func_text,
        (char**) (abc::Vec_PtrArray(input_names_abc)),
        input_names.size());

    output_pin->vFunc = *vFunc;
    ABC_FREE(vFunc);
    abc::Vec_PtrFree(input_names_abc);

    // ABC has a limit of one timing arc per input pin. Keep track
    // of pins that have already recieved a timing arc.
    std::unordered_set<std::string> pins;
    for (sta::TimingArcSet* arc_set :
         cell_port->libertyCell()->timingArcSets(nullptr, cell_port)) {
      // If the from pin is an output it means that this is
      // an output to output timing arc, and not something we
      // care about or can represent in ABC. Skip it.
      if (arc_set->from()->direction()->isOutput()) {
        continue;
      }

      std::string arc_pin_name = arc_set->from()->name();
      if (pins.find(arc_pin_name) != pins.end()) {
        continue;
      }
      pins.insert(arc_pin_name);

      abc::SC_Timings* timings = abc::Abc_SclTimingsAlloc();
      abc::Vec_PtrPush(&output_pin->vRTimings, timings);
      timings->pName = strdup(arc_pin_name.c_str());
      abc::SC_Timing* time_table = abc::Abc_SclTimingAlloc();
      abc::Vec_PtrPush(&timings->vTimings, time_table);

      sta::TimingSense timing_sense = arc_set->sense();
      if (timing_sense == sta::TimingSense::negative_unate) {
        time_table->tsense = abc::SC_TSense::sc_ts_Neg;
      } else if (timing_sense == sta::TimingSense::positive_unate) {
        time_table->tsense = abc::SC_TSense::sc_ts_Pos;
      } else if (timing_sense == sta::TimingSense::non_unate) {
        time_table->tsense = abc::SC_TSense::sc_ts_Non;
      } else {
        logger_->error(utl::CUT,
                       17,
                       "Unrecognized sta::TimingSense -> {}",
                       static_cast<int>(timing_sense));
      }

      sta::TimingArc* rise_arc = arc_set->arcTo(sta::RiseFall::rise());
      sta::TimingArc* fall_arc = arc_set->arcTo(sta::RiseFall::fall());

      sta::TimingModel* rise_model = rise_arc->model();
      sta::TimingModel* fall_model = fall_arc->model();

      const sta::GateTableModel* rise_gate_model
          = dynamic_cast<sta::GateTableModel*>(rise_model);
      const sta::GateTableModel* fall_gate_model
          = dynamic_cast<sta::GateTableModel*>(fall_model);

      if (!rise_gate_model || !fall_gate_model) {
        logger_->error(utl::CUT,
                       18,
                       "rise/fall Gate model is empty for cell {}. Need timing "
                       "information",
                       cell->name());
      }
      sta::Units* units = cell->libertyLibrary()->units();
      AbcPopulateAbcSurfaceFromSta(
          &time_table->pCellRise, rise_gate_model->delayModel(), units);
      AbcPopulateAbcSurfaceFromSta(
          &time_table->pCellFall, fall_gate_model->delayModel(), units);
      AbcPopulateAbcSurfaceFromSta(
          &time_table->pRiseTrans, rise_gate_model->slewModel(), units);
      AbcPopulateAbcSurfaceFromSta(
          &time_table->pFallTrans, fall_gate_model->slewModel(), units);
    }

    result.push_back(output_pin);
  }

  return result;
}

AbcLibraryFactory& AbcLibraryFactory::AddDbSta(sta::dbSta* db_sta)
{
  db_sta_ = db_sta;
  return *this;
}

AbcLibraryFactory& AbcLibraryFactory::AddResizer(rsz::Resizer* resizer)
{
  resizer_ = resizer;
  return *this;
}

AbcLibraryFactory& AbcLibraryFactory::SetCorner(sta::Corner* corner)
{
  corner_ = corner;
  return *this;
}

AbcLibrary AbcLibraryFactory::Build()
{
  if (!db_sta_) {
    logger_->error(utl::CUT, 19, "Build called with null sta library");
  }

  if (db_sta_->corners()->count() > 1 && !corner_) {
    logger_->error(
        utl::CUT, 20, "More than one corner is loaded, and no corner was set");
  }

  if (!corner_) {
    corner_ = db_sta_->corners()->corners()[0];
  }

  // Populate units from default liberty
  abc::SC_Lib* abc_library = abc::Abc_SclLibAlloc();
  sta::LibertyLibrary* default_library
      = db_sta_->network()->defaultLibertyLibrary();
  PopulateLibraryDetails(abc_library, default_library);

  // Grab cells from requested corner.
  std::vector<sta::LibertyCell*> liberty_cells
      = GetLibertyCellsFromCorner(corner_);

  PopulateAbcSclLibFromSta(
      abc_library, liberty_cells, default_library->units());

  abc::Abc_SclLibNormalize(abc_library);
  abc::Abc_SclHashCells(abc_library);
  abc::Abc_SclLinkCells(abc_library);

  return AbcLibrary(
      utl::UniquePtrWithDeleter<abc::SC_Lib>(
          abc_library, [](abc::SC_Lib* lib) { abc::Abc_SclLibFree(lib); }),
      logger_);
}

std::vector<sta::LibertyCell*> AbcLibraryFactory::GetLibertyCellsFromCorner(
    sta::Corner* corner)
{
  std::vector<sta::LibertyCell*> result;
  const sta::LibertySeq& libraries
      = corner->libertyLibraries(sta::MinMax::max());
  for (sta::LibertyLibrary* library : libraries) {
    sta::LibertyCellIterator cell_iterator(library);
    while (cell_iterator.hasNext()) {
      sta::LibertyCell* liberty_cell = cell_iterator.next();
      result.push_back(liberty_cell);
    }
  }

  return result;
}

void AbcLibraryFactory::PopulateLibraryDetails(abc::SC_Lib* sc_library,
                                               sta::LibertyLibrary* library)
{
  sta::Units* units = library->units();
  sta::Unit* time_unit = units->timeUnit();
  sta::Unit* cap_unit = units->capacitanceUnit();

  if (!sc_library->pName) {
    sc_library->pName = strdup(library->name());
  }

  if (!sc_library->pFileName) {
    sc_library->pFileName = strdup(library->filename());
  }

  sc_library->default_wire_load = nullptr;
  sc_library->default_wire_load_sel = nullptr;
  // TODO: Read and construct wireload tables

  // unit selection

  sc_library->unit_cap_fst = StaCapacitanceToAbc(cap_unit);
  sc_library->unit_cap_snd
      = ScaleAbbreviationToExponent(cap_unit->scaleAbbreviation());
  sc_library->unit_time = StaTimeUnitToAbcInt(time_unit);

  // defaults
  float slew = -1.0;
  bool max_slew_exists = false;
  library->defaultMaxSlew(slew, max_slew_exists);

  if (max_slew_exists) {
    sc_library->default_max_out_slew = time_unit->staToUser(slew);
  } else {
    sc_library->default_max_out_slew = -1.0;
  }
}

void AbcLibraryFactory::PopulateAbcSclLibFromSta(
    abc::SC_Lib* sc_library,
    std::vector<sta::LibertyCell*>& cells,
    sta::Units* units)
{
  // Loop through all of the cells in STA and create equivalents in
  // the ABC structure.
  for (sta::LibertyCell* cell : cells) {
    if (!isCompatibleWithAbc(cell, resizer_)) {
      continue;
    }

    abc::SC_Cell* abc_cell = abc::Abc_SclCellAlloc();
    abc_cell->Id = abc::SC_LibCellNum(sc_library);
    abc::Vec_PtrPush(&sc_library->vCells, abc_cell);

    abc_cell->pName = strdup(cell->name());
    abc_cell->area = cell->area();
    abc_cell->drive_strength = 0;

    // These are conditional leakages. Just average them
    // since abc can only accept a single value.
    sta::LeakagePowerSeq* leakage_powers = cell->leakagePowers();
    std::optional<float> average_leakage;
    for (sta::LeakagePower* power : *leakage_powers) {
      if (!average_leakage) {
        average_leakage = power->power();
        continue;
      }
      average_leakage = average_leakage.value() + power->power();
    }

    bool leakage_power_exists;
    float leakage_power = 0;
    cell->leakagePower(leakage_power, leakage_power_exists);
    sta::Unit* power_unit = units->powerUnit();
    if (leakage_power_exists) {
      abc_cell->leakage = power_unit->staToUser(leakage_power);
    } else if (average_leakage) {
      // We know we'll always have at least one leakage power since average
      // is present.
      abc_cell->leakage = power_unit->staToUser(average_leakage.value()
                                                / leakage_powers->size());
    } else {
      logger_->warn(utl::CUT,
                    21,
                    "Leakage power doesn't exist for cell {}",
                    cell->name());
    }

    // Input Pin Creation
    std::vector<abc::SC_Pin*> input_pins = CreateAbcInputPins(cell);
    std::vector<std::string> input_pin_names;
    input_pin_names.reserve(input_pins.size());
    abc_cell->n_inputs = input_pins.size();
    for (abc::SC_Pin* pin : input_pins) {
      abc::Vec_PtrPush(&abc_cell->vPins, pin);
      input_pin_names.emplace_back(pin->pName);
    }

    std::vector<abc::SC_Pin*> output_pins
        = CreateAbcOutputPins(cell, input_pin_names);
    abc_cell->n_outputs = output_pins.size();
    for (abc::SC_Pin* pin : output_pins) {
      abc::Vec_PtrPush(&abc_cell->vPins, pin);
    }
  }
}

float AbcLibraryFactory::StaCapacitanceToAbc(sta::Unit* cap_unit)
{
  // Abc stores capacitance as two variables x(float) * 10^y(int)
  // This function extracts the x variable from STA scale value.
  float cap_scale = cap_unit->scale();
  int exponent_y
      = std::floor(std::abs(std::log10(std::abs(cap_unit->scale()))));
  return cap_scale * std::pow(10.0, exponent_y);
}

int AbcLibraryFactory::StaTimeUnitToAbcInt(sta::Unit* time_unit)
{
  float scale = std::abs(std::log10(std::abs(time_unit->scale())));
  int abc_time_unit = std::floor(scale);

  if (abc_time_unit < 9 || abc_time_unit > 12) {
    logger_->error(
        utl::CUT, 22, "Time unit {} outside abc range [1e-9, 1e-12]", scale);
  }

  return abc_time_unit;
}

int AbcLibraryFactory::ScaleAbbreviationToExponent(
    const std::string& scale_abbreviation)
{
  if (scale_abbreviation == "m") {
    return 3;
  }
  if (scale_abbreviation == "u") {
    return 6;
  }
  if (scale_abbreviation == "n") {
    return 9;
  }
  if (scale_abbreviation == "p") {
    return 12;
  }
  if (scale_abbreviation == "f") {
    return 15;
  }

  logger_->error(
      utl::CUT, 23, "Can't convert scale abbreviation {}", scale_abbreviation);
}

bool AbcLibrary::IsSupportedCell(const std::string& cell_name)
{
  if (supported_cells_.empty()) {
    int num_gates = abc::SC_LibCellNum(abc_library_.get());
    for (int i = 0; i < num_gates; i++) {
      abc::SC_Cell* cell = abc::SC_LibCell(abc_library_.get(), i);
      if (cell->n_outputs != 1) {
        continue;
      }
      supported_cells_.insert(cell->pName);
    }
  }
  return supported_cells_.find(cell_name) != supported_cells_.end();
}

void AbcLibrary::InitializeConstGates()
{
  const_gates_initalized_ = true;
  for (int i = 0; i < abc::SC_LibCellNum(abc_library_.get()); i++) {
    abc::SC_Cell* current_cell = abc::SC_LibCell(abc_library_.get(), i);
    if (current_cell->n_inputs != 0) {
      continue;
    }
    for (int i = current_cell->n_inputs;
         i < current_cell->n_inputs + current_cell->n_outputs;
         i++) {
      abc::SC_Pin* pin = abc::SC_CellPin(current_cell, 0);
      // In ABC land we store the right hand side of the truth
      // table in a bit vector sort of thing. Since the const
      // cell has less than 6 inputs its truth table should be
      // in entry zero.
      abc::word constant_0 = 0;
      abc::word constant_1 = ~constant_0;
      abc::word truth_table = abc::Vec_WrdEntry(&pin->vFunc, 0);
      if (truth_table == constant_0) {
        const0_gates_.insert(current_cell->pName);
      }

      if (truth_table == constant_1) {
        const1_gates_.insert(current_cell->pName);
      }
    }
  }
}

// Find cell matching truth table for constant cells either 0 or 1
// where 1 is represented as all 1s in binary.
std::pair<abc::SC_Cell*, abc::SC_Pin*> FindConstantCell(abc::SC_Lib* library,
                                                        abc::word constant)
{
  std::pair<abc::SC_Cell*, abc::SC_Pin*> result = {nullptr, nullptr};
  std::optional<float> min_area;
  for (int i = 0; i < abc::SC_LibCellNum(library); i++) {
    abc::SC_Cell* current_cell = abc::SC_LibCell(library, i);
    if (current_cell->n_inputs != 0) {
      continue;
    }

    for (int i = current_cell->n_inputs;
         i < current_cell->n_inputs + current_cell->n_outputs;
         i++) {
      abc::SC_Pin* pin = abc::SC_CellPin(current_cell, i);
      // In ABC land we store the right hand side of the truth
      // table in a bit vector sort of thing. Since the const
      // cell has less than 6 inputs its truth table should be
      // in entry zero.
      abc::word truth_table = abc::Vec_WrdEntry(&pin->vFunc, 0);

      // If the truth table matches the constant value then this is
      // the cell we are looking for. Try to choose the cell with the
      // smallest area.
      if (truth_table == constant) {
        if (min_area.has_value()) {
          if (current_cell->area < min_area.value()) {
            result.first = current_cell;
            result.second = pin;
            min_area = current_cell->area;
          }
        } else {
          result.first = current_cell;
          result.second = pin;
          min_area = current_cell->area;
        }
      }
    }
  }

  return result;
}

std::pair<abc::SC_Cell*, abc::SC_Pin*> AbcLibrary::ConstantZeroCell()
{
  if (const0_cell_) {
    return const0_cell_.value();
  }
  abc::word constant_0 = 0;
  const0_cell_ = FindConstantCell(abc_library_.get(), constant_0);
  return const0_cell_.value();
}

std::pair<abc::SC_Cell*, abc::SC_Pin*> AbcLibrary::ConstantOneCell()
{
  if (const1_cell_) {
    return const1_cell_.value();
  }
  abc::word constant_0 = 0;
  abc::word constant_1 = ~constant_0;
  const1_cell_ = FindConstantCell(abc_library_.get(), constant_1);
  return const1_cell_.value();
}

bool AbcLibrary::IsConst0Cell(const std::string& cell_name)
{
  if (!const_gates_initalized_) {
    InitializeConstGates();
  }
  return const0_gates_.find(cell_name) != const0_gates_.end();
}
bool AbcLibrary::IsConst1Cell(const std::string& cell_name)
{
  if (!const_gates_initalized_) {
    InitializeConstGates();
  }
  return const1_gates_.find(cell_name) != const1_gates_.end();
}

bool AbcLibrary::IsConstCell(const std::string& cell_name)
{
  return IsConst1Cell(cell_name) || IsConst0Cell(cell_name);
}

static bool abc_initialized = false;

void abcInit()
{
  if (!abc_initialized) {
    abc::Abc_Start();
    abc_initialized = true;
  }
}

void abcStop()
{
  if (abc_initialized) {
    abc::Abc_Stop();
  }
}

}  // namespace cut
