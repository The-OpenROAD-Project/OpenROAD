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
  void create_power_domain_cmd(char* name)
  {
    odb::dbDatabase* db = getOpenRoad()->getDb();
    create_power_domain(getOpenRoad()->getLogger(), db->getChip()->getBlock(), name); 
  }

  void update_power_domain_cmd(char* name, char* element)
  {
    odb::dbDatabase* db = getOpenRoad()->getDb();
    upf::update_power_domain(
        getOpenRoad()->getLogger(), db->getChip()->getBlock(), name, element); 
  }

  void create_logic_port_cmd(char* name, char* direction)
  {
    odb::dbDatabase* db = getOpenRoad()->getDb();
    upf::create_logic_port(
        getOpenRoad()->getLogger(), db->getChip()->getBlock(), name, direction); 
  }

  void create_power_switch_cmd(
      char* name, char* domain, char* out_port, char* in_port)
  {
    odb::dbDatabase* db = getOpenRoad()->getDb();
    upf::create_power_switch(
        getOpenRoad()->getLogger(), db->getChip()->getBlock(), name, domain, out_port, in_port); 
  }

  void update_power_switch_control_cmd(char* name, char* port)
  {
    odb::dbDatabase* db = getOpenRoad()->getDb();
    upf::update_power_switch_control(
        getOpenRoad()->getLogger(), db->getChip()->getBlock(), name, port); 
  }

  void update_power_switch_on_cmd(char* name, char* port)
  {
    odb::dbDatabase* db = getOpenRoad()->getDb();
    upf::update_power_switch_on(
        getOpenRoad()->getLogger(), db->getChip()->getBlock(), name, port); 
  }

  void set_isolation_cmd(char* name,
                         char* domain,
                         bool update,
                         char* applies_to,
                         char* clamp_value,
                         char* isolation_signal,
                         char* isolation_sense,
                         char* location)
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

  void use_interface_cell_cmd(char* domain,
                              char* strategy,
                              char* lib_cell)
  {
    odb::dbDatabase* db = getOpenRoad()->getDb();
    upf::use_interface_cell(getOpenRoad()->getLogger(),
                            db->getChip()->getBlock(), 
                            domain,
                            strategy,
                            lib_cell);
  }

  void set_domain_area_cmd(char* domain,
                           float x1,
                           float y1,
                           float x2,
                           float y2)
  {

    odb::dbDatabase* db = getOpenRoad()->getDb();
    upf::set_domain_area(getOpenRoad()->getLogger(),
                        db->getChip()->getBlock(), 
                        domain,
                        x1,
                        y1,
                        x2,
                        y2);
  }
%}  // inline
