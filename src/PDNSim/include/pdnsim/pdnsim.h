#ifndef __PDNSim_HEADER__
#define __PDNSim_HEADER__


#include <string>

namespace odb {
  class dbDatabase;
}
namespace sta {
  class dbSta;
}

namespace pdnsim {

class PDNSim
{
  public:
    PDNSim();
    ~PDNSim();

    void init();
    void reset();

    void setDb(odb::dbDatabase* odb);
    void setSta(sta::dbSta* dbSta);
    
    void import_vsrc_cfg(std::string vsrc);
    void import_out_file(std::string out_file);
    void import_spice_out_file(std::string out_file);
    void set_power_net(std::string net);

    int analyze_power_grid();
    void write_pg_spice();

    int check_connectivity();

  private:
    odb::dbDatabase* _db;
    sta::dbSta* _sta;
    std::string _vsrc_loc;
    std::string _out_file;
    std::string _spice_out_file;
    std::string _power_net;


};
}

#endif
