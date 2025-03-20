#pragma once
#include <functional>
#include <memory>
#include <vector>
#include <array>            // For std::array
#include "odb/db.h"

namespace odb {
class dbMaster;
}

namespace sta {
class dbNetwork;
class LibertyPort;
}

namespace utl {
class Logger;
}

namespace ram {

using utl::Logger;

class Element;
class Layout;

class RamGen {
 public:
  RamGen();
  void init(odb::dbDatabase* db, sta::dbNetwork* network, Logger* logger);

  void generate(int bytes_per_word,
                int word_count,
                int read_ports,
                odb::dbMaster* storage_cell,
                odb::dbMaster* tristate_cell,
                odb::dbMaster* inv_cell);

 private:
  // Technology/Cell Functions
  void findMasters();
  odb::dbMaster* findMaster(const std::function<bool(sta::LibertyPort*)>& match,
                            const char* name);

  // Net/Instance/Term Creation
  odb::dbNet* makeNet(const std::string& prefix, const std::string& name);
  odb::dbInst* makeInst(Layout* layout,
                        const std::string& prefix,
                        const std::string& name,
                        odb::dbMaster* master,
                        const std::vector<std::pair<std::string, odb::dbNet*>>& connections);
  odb::dbNet* makeBTerm(const std::string& name);

  // Hierarchical Components
  std::unique_ptr<Element> make_decoder(const std::string& prefix,
                                        int address_bits,
                                        const std::vector<odb::dbNet*>& inputs);

  std::unique_ptr<Element> make_bit(const std::string& prefix,
                                    int read_ports,
                                    odb::dbNet* clock,
                                    const std::vector<odb::dbNet*>& select,
                                    odb::dbNet* data_input,
                                    std::vector<odb::dbNet*>& data_output);

  std::unique_ptr<Element> make_byte(const std::string& prefix,
                                     int read_ports,
                                     odb::dbNet* clock,
                                     odb::dbNet* write_enable,
                                     const std::vector<odb::dbNet*>& selects,
                                     const std::array<odb::dbNet*, 8>& data_input,
                                     const std::vector<std::array<odb::dbNet*, 8>>& data_output);

  // NEW: Word/RAM8/RAM32 creation
  std::unique_ptr<Element> make_word(const std::string& prefix,
                                     int read_ports,
                                     odb::dbNet* clock,
                                     const std::vector<odb::dbNet*>& we_per_byte,
                                     odb::dbNet* sel,
                                     const std::array<odb::dbNet*, 32>& data_input,
                                     const std::vector<std::array<odb::dbNet*, 32>>& data_output);

  std::unique_ptr<Element> make_ram8(const std::string& prefix,
                                     int read_ports,
                                     odb::dbNet* clock,
                                     const std::vector<odb::dbNet*>& we_per_word,
                                     const std::vector<odb::dbNet*>& addr3bit,
                                     const std::array<odb::dbNet*, 32>& data_input,
                                     const std::vector<std::array<odb::dbNet*, 32>>& data_output);

  std::unique_ptr<Element> make_ram32(const std::string& prefix,
                                      int read_ports,
                                      odb::dbNet* clock,
                                      const std::vector<odb::dbNet*>& we_32,
                                      const std::vector<odb::dbNet*>& addr5bit,
                                      const std::array<odb::dbNet*, 32>& data_input,
                                      const std::vector<std::array<odb::dbNet*, 32>>& data_output);

  std::unique_ptr<Element> make_ram8_8bit(const std::string& prefix,
                                                int read_ports,
                                                odb::dbNet* clock,
                                                const std::vector<odb::dbNet*>& we_8,
                                                const std::vector<odb::dbNet*>& addr3,
                                                const std::array<odb::dbNet*, 8>& data_input,
                                                const std::vector<std::array<odb::dbNet*, 8>>& data_output);

  odb::dbInst* createBufferInstance(odb::dbNet* net);

  // Database pointers
  odb::dbDatabase* db_;
  odb::dbBlock* block_;
  sta::dbNetwork* network_;
  Logger* logger_;

  // Technology cells
  odb::dbMaster* storage_cell_;
  odb::dbMaster* tristate_cell_;
  odb::dbMaster* inv_cell_;
  odb::dbMaster* and2_cell_;
  odb::dbMaster* clock_gate_cell_;
};

}  // namespace ram
