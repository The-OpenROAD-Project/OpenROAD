#pragma once
#include <functional>
#include <memory>
#include <vector>
#include <array>
#include <map>
#include <string>
#include <set>
#include <optional>
#include "odb/db.h"

namespace odb {
class dbMaster;
class dbSite;
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

// Forward declarations
class Element;
class Layout;
class Row;
class MemoryConfig;
class LayoutBuilder;
class HierarchyBuilder;

// Memory type enumeration
enum MemoryType {
  RAM_1RW,     // Single Read/Write Port
  RAM_1RW1R,   // One Read/Write Port, One Read Port
  RAM_2R1W     // Two Read Ports, One Write Port (for Register Files)
};

// Row placement configuration
struct RowConfig {
  int rows_count;
  double site_width;
  double site_height;
  double tap_distance;
  std::string tap_cell_pattern;
  std::vector<std::string> filler_cell_patterns;
  
  RowConfig() : rows_count(0), site_width(0), site_height(0), tap_distance(0) {}
};

// Class to represent a memory placement row (inspired by DFFRAM's Row class)
class Row {
public:
  Row(int ordinal, odb::Point origin, odb::dbSite* site, int sites_count);
  
  // Place an instance on this row
  void place(odb::dbInst* inst);
  
  // Add a tap cell at the current position
  void addTap(odb::dbBlock* block, odb::dbMaster* tap_cell);
  
  // Fill row with filler cells
  void fill(odb::dbBlock* block, const std::vector<odb::dbMaster*>& filler_cells);
  
  // Static method to fill a range of rows
  static void fillRows(std::vector<Row>& rows, int from_index, int to_index,
                       odb::dbBlock* block, const std::vector<odb::dbMaster*>& filler_cells);
  
  // Getters
  int getOrdinal() const { return ordinal_; }
  odb::Point getCurrentPosition() const { return current_position_; }
  int getSiteCount() const { return sites_count_; }
  bool isFull() const { return current_sites_ >= sites_count_; }
  int getWidth() const { return current_position_.getX() - origin_.getX(); }
  odb::Point getOrigin() const { return origin_; }
  
private:
  int ordinal_;                      // Row number
  odb::Point origin_;                // Starting position
  odb::Point current_position_;      // Current placement position
  odb::dbSite* site_;                // Site template
  int site_width_;                   // Width of a single site
  int sites_count_;                  // Total sites in row
  int current_sites_;                // Sites used so far
  int cell_counter_;                 // Counter for unique cell naming
  int fill_counter_;                 // Counter for filler cells
  int tap_counter_;                  // Counter for tap cells
  int since_last_tap_;               // Sites since last tap cell
};

// Enhanced element class for building blocks
class Element {
public:
  enum ElementType {
    INSTANCE,
    LAYOUT,
    BIT,
    BYTE,
    WORD,
    DECODER,
    RAM8,
    RAM32,
    RAM128,
    RAM512,
    REGISTER_FILE
  };

  Element() = delete;
  
  // Constructors
  explicit Element(odb::dbInst* inst, ElementType type = INSTANCE);
  explicit Element(std::unique_ptr<Layout> layout, ElementType type = LAYOUT);
  
  // Position this element at the given origin
  odb::Rect position(odb::Point origin);
  
  // Position this element on a specific row
  odb::Rect placeOnRow(Row& row);
  
  // Get properties
  odb::dbInst* getInstance() const { return inst_; }
  Layout* getLayout() const { return layout_.get(); }
  ElementType getType() const { return type_; }
  int getWidth() const;
  int getHeight() const;
  std::string getName() const;
  
  // Add a child element (for hierarchical elements)
  void addChild(std::unique_ptr<Element> child);
  const std::vector<std::unique_ptr<Element>>& getChildren() const { return children_; }
  
private:
  odb::dbInst* inst_ = nullptr;
  std::unique_ptr<Layout> layout_;
  ElementType type_;
  std::vector<std::unique_ptr<Element>> children_;
};

// Enhanced layout class with DFFRAM-like placement capabilities
class Layout {
public:
  Layout(odb::Orientation2D orientation = odb::horizontal);
  
  // Add an element to the layout
  void addElement(std::unique_ptr<Element> element);
  
  // Position the layout at the given origin
  odb::Rect position(odb::Point origin);
  
  // DFFRAM-inspired placement methods
  void optimizeLayout(odb::dbBlock* block, const RowConfig& config);
  void createAndPlaceRows(odb::dbBlock* block, const RowConfig& config);
  void placeBitCells();
  void placeDecoders();
  void placeControlLogic();
  void fillWithTapCells(odb::dbBlock* block, odb::dbMaster* tap_cell, int tap_distance);
  
  // Grouping elements by type
  std::vector<Element*> findElementsByType(Element::ElementType type);
  
  // Get properties
  std::vector<std::unique_ptr<Element>>& getElements() { return elements_; }
  int getEstimatedWidth() const;
  int getEstimatedHeight() const;
  odb::Orientation2D getOrientation() const { return orientation_; }
  void setRows(std::vector<Row>&& rows) { rows_ = std::move(rows); }
  const std::vector<Row>& getRows() const { return rows_; }
  
private:
  odb::Orientation2D orientation_;
  std::vector<std::unique_ptr<Element>> elements_;
  std::vector<Row> rows_;
  
  // Cached dimensions
  mutable int estimated_width_ = -1;
  mutable int estimated_height_ = -1;
  
  // Helper methods
  void calculateEstimatedDimensions() const;
  void alignElementsInRows();
};

// Memory configuration class
class MemoryConfig {
public:
  MemoryConfig(int bytes_per_word, int word_count, int read_ports, MemoryType type)
    : bytes_per_word_(bytes_per_word),
      word_count_(word_count),
      read_ports_(read_ports),
      type_(type) {}
      
  int getBytesPerWord() const { return bytes_per_word_; }
  int getWordCount() const { return word_count_; }
  int getReadPorts() const { return read_ports_; }
  MemoryType getType() const { return type_; }
  
  // Calculate total size in bits
  int getTotalBits() const { return bytes_per_word_ * 8 * word_count_; }
  
  // Calculate address bits
  int getAddressBits() const { return calculateBits(word_count_); }
  
  // Helper method
  static int calculateBits(int value) {
    if (value <= 0) return 0;
    return (int)std::log2(value);
  }
  
private:
  int bytes_per_word_;
  int word_count_;
  int read_ports_;
  MemoryType type_;
};

// Hierarchy builder class
class HierarchyBuilder {
public:
  HierarchyBuilder(odb::dbBlock* block, const MemoryConfig& config, Logger* logger, sta::dbNetwork* network = nullptr);
  
  void setMasters(odb::dbMaster* storage_cell,
    odb::dbMaster* tristate_cell,
    odb::dbMaster* inv_cell,
    odb::dbMaster* and2_cell,
    odb::dbMaster* clock_gate_cell,
    odb::dbMaster* mux2_cell = nullptr);


  // Build the memory hierarchy (similar to DFFRAM's create_hierarchy)
  std::unique_ptr<Element> buildHierarchy();
  
  // Building block creation methods
  std::unique_ptr<Element> createBit(const std::string& prefix, 
                                    int read_ports,
                                    odb::dbNet* clock,
                                    odb::dbNet* data_in,
                                    const std::vector<odb::dbNet*>& select,
                                    std::vector<odb::dbNet*>& data_out);
                                    
  std::unique_ptr<Element> createByte(const std::string& prefix,
                                     int read_ports,
                                     odb::dbNet* clock,
                                     odb::dbNet* write_enable,
                                     const std::vector<odb::dbNet*>& selects,
                                     const std::array<odb::dbNet*, 8>& data_input,
                                     const std::vector<std::array<odb::dbNet*, 8>>& data_output);
                                     
  std::unique_ptr<Element> createWord(const std::string& prefix,
                                     int read_ports,
                                     odb::dbNet* clock,
                                     const std::vector<odb::dbNet*>& we_per_byte,
                                     odb::dbNet* sel,
                                     const std::array<odb::dbNet*, 32>& data_input,
                                     const std::vector<std::array<odb::dbNet*, 32>>& data_output);
                                     
  std::unique_ptr<Element> createRAM8(const std::string& prefix,
                                     int read_ports,
                                     odb::dbNet* clock,
                                     const std::vector<odb::dbNet*>& we_per_word,
                                     const std::vector<odb::dbNet*>& addr3bit,
                                     const std::array<odb::dbNet*, 32>& data_input,
                                     const std::vector<std::array<odb::dbNet*, 32>>& data_output);
                                     
  std::unique_ptr<Element> createRAM32(const std::string& prefix,
                                      int read_ports,
                                      odb::dbNet* clock,
                                      const std::vector<odb::dbNet*>& we_32,
                                      const std::vector<odb::dbNet*>& addr5bit,
                                      const std::array<odb::dbNet*, 32>& data_input,
                                      const std::vector<std::array<odb::dbNet*, 32>>& data_output);
                                      
  std::unique_ptr<Element> createRAM128(const std::string& prefix,
                                       int read_ports,
                                       odb::dbNet* clock,
                                       const std::vector<odb::dbNet*>& we_128,
                                       const std::vector<odb::dbNet*>& addr7bit,
                                       const std::array<odb::dbNet*, 32>& data_input,
                                       const std::vector<std::array<odb::dbNet*, 32>>& data_output);
                                       
  std::unique_ptr<Element> createRAM512(const std::string& prefix,
                                       int read_ports,
                                       odb::dbNet* clock,
                                       const std::vector<odb::dbNet*>& we_512,
                                       const std::vector<odb::dbNet*>& addr9bit,
                                       const std::array<odb::dbNet*, 32>& data_input,
                                       const std::vector<std::array<odb::dbNet*, 32>>& data_output);
                                       
  std::unique_ptr<Element> createDecoder(const std::string& prefix,
                                        int address_bits,
                                        const std::vector<odb::dbNet*>& inputs);
                                        
  std::unique_ptr<Element> createReadPort(const std::string& prefix,
                                         odb::dbNet* clock,
                                         odb::dbNet* read_enable,
                                         const std::vector<odb::dbNet*>& addr);
                                         
  // Net creation methods
  odb::dbNet* createNet(const std::string& name);
  odb::dbNet* createBTermNet(const std::string& name);
  odb::dbInst* createInst(const std::string& name, odb::dbMaster* master,
                         const std::vector<std::pair<std::string, odb::dbNet*>>& connections);
                         
private:
  odb::dbBlock* block_;
  const MemoryConfig& config_;
  Logger* logger_;
  sta::dbNetwork* network_;
  std::map<std::string, odb::dbNet*> nets_cache_;
  std::map<std::string, odb::dbMaster*> masters_cache_;
  bool masters_are_set_ = false;
  
  // Find masters
  void findMasters();

  odb::dbMaster* findMaster(const std::function<bool(sta::LibertyPort*)>& match, const char* name);
  odb::dbMaster* getMaster(const std::string& name);
  
  // Cache of commonly used masters
  odb::dbMaster* storage_cell_ = nullptr;
  odb::dbMaster* tristate_cell_ = nullptr;
  odb::dbMaster* inv_cell_ = nullptr;
  odb::dbMaster* and2_cell_ = nullptr;
  odb::dbMaster* clock_gate_cell_ = nullptr;
  odb::dbMaster* mux2_cell_ = nullptr;
};

// In ram.h - Updated LayoutBuilder class declaration

class LayoutBuilder {
public:
  // Updated constructor with pre-found masters
  LayoutBuilder(odb::dbBlock* block, const MemoryConfig& config, 
               const RowConfig& row_config, Logger* logger,
               odb::dbMaster* tap_cell = nullptr,
               const std::vector<odb::dbMaster*>& filler_cells = {});
  
  void buildLayout(std::unique_ptr<Element> hierarchy);
  void optimizeLayout();
  bool writeToDefFile(const std::string& filename) const;
  
  // Get the built layout
  Layout* getLayout() const { return layout_.get(); }
  
private:
  odb::dbBlock* block_;
  const MemoryConfig& config_;
  const RowConfig& row_config_;
  Logger* logger_;
  std::unique_ptr<Layout> layout_;
  
  // NEW: Store provided masters
  odb::dbMaster* provided_tap_cell_ = nullptr;
  std::vector<odb::dbMaster*> provided_filler_cells_;
  
  // NEW: Database validation helper
  bool validateDatabase() const;
  
  // Updated to return success/failure
  bool setupRows();
  void placeBitCells();
  void placeDecoders();
  void placeControlLogic();
  void fillEmptySpaces();
  
  // Search methods (used as fallback)
  std::vector<odb::dbMaster*> findFillerCells();
  odb::dbMaster* findTapCell();
};




// Main RAM generator class
class RamGen {
 public:
  RamGen();
  void init(odb::dbDatabase* db, sta::dbNetwork* network, Logger* logger);

  // Original method for backward compatibility
  void generate(int bytes_per_word,
                int word_count,
                int read_ports,
                odb::dbMaster* storage_cell,
                odb::dbMaster* tristate_cell,
                odb::dbMaster* inv_cell);

  // Enhanced method with DFFRAM-like capabilities
  void generateDFFRAM(int bytes_per_word,
                     int word_count,
                     int read_ports,
                     MemoryType mem_type,
                     const std::string& output_def = "",
                     bool optimize_layout = true);

  // Register file generation
  void generateRegisterFile(int word_count,
                           int word_width,
                           const std::string& output_def = "");

  // Helper methods
  const char* getMemoryTypeName(MemoryType type) const;
  
  // Access to current block
  odb::dbBlock* getCurrentBlock() const { return block_; }

 private:
  // Initialize technology cells
  void setupTechnologyCells();
  
  // Robust database validation - new method
  bool validateDatabase(const char* caller_name);
  
  // Create output DEF file
  bool writeDefFile(const std::string& filename) const;
  
  // Database pointers
  odb::dbDatabase* db_;
  odb::dbBlock* block_;
  sta::dbNetwork* network_;
  Logger* logger_;
  
  // Row configuration
  RowConfig row_config_;
  
  // Masters cache
  std::map<std::string, odb::dbMaster*> masters_;
  
  // Frequently used masters
  odb::dbMaster* storage_cell_;
  odb::dbMaster* tristate_cell_;
  odb::dbMaster* inv_cell_;
  odb::dbMaster* and2_cell_;
  odb::dbMaster* clock_gate_cell_;
  odb::dbMaster* mux2_cell_;
};

}  // namespace ram