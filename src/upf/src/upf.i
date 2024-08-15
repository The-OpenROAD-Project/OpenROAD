%{
#include "odb/db.h"
#include "upf/upf.h"
#include "ord/OpenRoad.hh"

  namespace ord {
  OpenRoad* getOpenRoad();
  }

  using ord::getOpenRoad;
  using namespace upf;
%}



%inline %{
  void create_power_domain_cmd(const char* name)
  {
    odb::dbDatabase* db = getOpenRoad()->getDb();
    create_power_domain(getOpenRoad()->getLogger(), db->getChip()->getBlock(), name); 
  }

  void update_power_domain_cmd(const char* name, const char* element)
  {
    odb::dbDatabase* db = getOpenRoad()->getDb();
    upf::update_power_domain(
        getOpenRoad()->getLogger(), db->getChip()->getBlock(), name, element); 
  }

  void create_logic_port_cmd(const char* name, const char* direction)
  {
    odb::dbDatabase* db = getOpenRoad()->getDb();
    upf::create_logic_port(
        getOpenRoad()->getLogger(), db->getChip()->getBlock(), name, direction); 
  }

  void create_power_switch_cmd(
      const char* name, const char* domain)
  {
    odb::dbDatabase* db = getOpenRoad()->getDb();
    upf::create_power_switch(
        getOpenRoad()->getLogger(), db->getChip()->getBlock(), name, domain); 
  }

  void update_power_switch_control_cmd(const char* name, const char* port, const char* net)
  {
    odb::dbDatabase* db = getOpenRoad()->getDb();
    upf::update_power_switch_control(
        getOpenRoad()->getLogger(), db->getChip()->getBlock(), name, port, net); 
  }

  void update_power_switch_ack_cmd(const char* name, const char* port, const char* net, const char* boolean)
  {
    odb::dbDatabase* db = getOpenRoad()->getDb();
    upf::update_power_switch_acknowledge(
        getOpenRoad()->getLogger(), db->getChip()->getBlock(), name, port, net, boolean); 
  }

  void update_power_switch_on_cmd(const char* name, const char* state, const char* port, const char* boolean)
  {
    odb::dbDatabase* db = getOpenRoad()->getDb();
    upf::update_power_switch_on(
        getOpenRoad()->getLogger(), db->getChip()->getBlock(), name, state, port, boolean); 
  }


  void update_power_switch_input_cmd(const char* name, const char* port, const char* net)
  {
    odb::dbDatabase* db = getOpenRoad()->getDb();
    upf::update_power_switch_input(
        getOpenRoad()->getLogger(), db->getChip()->getBlock(), name, port, net); 
  }

  void update_power_switch_output_cmd(const char* name, const char* port, const char* net)
  {
    odb::dbDatabase* db = getOpenRoad()->getDb();
    upf::update_power_switch_output(
        getOpenRoad()->getLogger(), db->getChip()->getBlock(), name, port, net); 
  }


  void set_isolation_cmd(const char* name,
                         const char* domain,
                         bool update,
                         const char* applies_to,
                         const char* clamp_value,
                         const char* isolation_signal,
                         const char* isolation_sense,
                         const char* location)
  {
    odb::dbDatabase* db = getOpenRoad()->getDb();
    upf::set_isolation(getOpenRoad()->getLogger(),
                            db->getChip()->getBlock(), 
                            name,
                            domain,
                            update,
                            applies_to,
                            clamp_value,
                            isolation_signal,
                            isolation_sense,
                            location);
  }

  void use_interface_cell_cmd(const char* domain,
                              const char* strategy,
                              const char* lib_cell)
  {
    odb::dbDatabase* db = getOpenRoad()->getDb();
    upf::use_interface_cell(getOpenRoad()->getLogger(),
                            db->getChip()->getBlock(), 
                            domain,
                            strategy,
                            lib_cell);
  }

  void set_domain_area_cmd(const char* domain,
                           const odb::Rect& area)
  {

    odb::dbDatabase* db = getOpenRoad()->getDb();
    upf::set_domain_area(getOpenRoad()->getLogger(),
                         db->getChip()->getBlock(), 
                         domain,
                         area);
  }

  void set_power_switch_cell(const char* name, const char* cell)
  {
    odb::dbDatabase* db = getOpenRoad()->getDb();
    auto libs = db->getLibs();
    odb::dbMaster* master = nullptr;
    for (auto lib : libs) {
      master = lib->findMaster(cell);
      if (master) {
        break;
      }
    }

    if (!master) {
      getOpenRoad()->getLogger()->error(utl::UPF,
                                        43,
                                        "Cannot find master {}",
                                        cell);
      return;
    }
    upf::update_power_switch_cell(
        getOpenRoad()->getLogger(), db->getChip()->getBlock(), name, master); 
  }

  void set_power_switch_port_map(const char* name, const char* model_port, const char* switch_port)
  {
    odb::dbDatabase* db = getOpenRoad()->getDb();
    upf::update_power_switch_port_map(
        getOpenRoad()->getLogger(), db->getChip()->getBlock(), name, model_port, switch_port); 
  }

  void create_or_update_level_shifter_cmd(const char* name,
                                    const char* domain,
                                    const char* source,
                                    const char* sink,
                                    const char* use_functional_equivalence,
                                    const char* applies_to,
                                    const char* applies_to_boundary,
                                    const char* rule,
                                    const char* threshold,
                                    const char* no_shift,
                                    const char* force_shift,
                                    const char* location,
                                    const char* input_supply,
                                    const char* output_supply,
                                    const char* internal_supply,
                                    const char* name_prefix,
                                    const char* name_suffix,
                                    bool update)
  {
    odb::dbDatabase* db = getOpenRoad()->getDb();
    upf::create_or_update_level_shifter(
        getOpenRoad()->getLogger(), db->getChip()->getBlock(), name, domain, source, sink, use_functional_equivalence, applies_to, applies_to_boundary, rule, threshold, no_shift, force_shift, location, input_supply, output_supply, internal_supply, name_prefix, name_suffix, update); 
  }

  void add_level_shifter_element_cmd(const char* level_shifter_name, const char* element)
  {
    odb::dbDatabase* db = getOpenRoad()->getDb();
    upf::add_level_shifter_element(
        getOpenRoad()->getLogger(), db->getChip()->getBlock(), level_shifter_name, element); 
  }

  void exclude_level_shifter_element_cmd(const char* level_shifter_name, const char* exclude_element)
  {
    odb::dbDatabase* db = getOpenRoad()->getDb();
    upf::exclude_level_shifter_element(
        getOpenRoad()->getLogger(), db->getChip()->getBlock(), level_shifter_name, exclude_element); 
  }

  void handle_level_shifter_instance_cmd(const char* level_shifter_name, const char* instance, const char* port)
  {
    odb::dbDatabase* db = getOpenRoad()->getDb();
    upf::handle_level_shifter_instance(
        getOpenRoad()->getLogger(), db->getChip()->getBlock(), level_shifter_name, instance, port); 
  }

  void set_domain_voltage_cmd(const char* domain, float voltage)
  {
    odb::dbDatabase* db = getOpenRoad()->getDb();
    upf::set_domain_voltage(
        getOpenRoad()->getLogger(), db->getChip()->getBlock(), domain, voltage); 
  }

  void set_level_shifter_cell_cmd(const char* shifter, const char* cell, const char* input, const char* output){
    odb::dbDatabase* db = getOpenRoad()->getDb();
    upf::set_level_shifter_cell(
        getOpenRoad()->getLogger(), db->getChip()->getBlock(), shifter, cell, input, output); 
  }

  void write_upf_cmd(const char* file) {
    odb::dbDatabase* db = getOpenRoad()->getDb();
    upf::write_upf(getOpenRoad()->getLogger(), db->getChip()->getBlock(), file);
  }
%}  // inline
