# insert_buffer

The `insert_buffer` command is used to manually insert a buffer into the design. It supports three main buffering scenarios: after a driver pin, before a single load pin, and before multiple load pins.

It provides two sets of APIs: the high-level `Resizer` APIs and the low-level 
`dbNet` APIs. 
Recommend using `Resizer` APIs when the `Resizer` module is available because 
it provides more features.

## Features
- Standard APIs to insert a new buffer: Do not recommend using raw
  `dbInst::create()` API to insert a new buffer.
- Support hierarchical flow: Handles `dbModNet` connections under the hood.
- Support legalization: Legalize the buffer position when `Resizer` APIs are
  used.
- Support GUI: Update the GUI when `Resizer` APIs are used.

## Tcl Command Usage

```tcl
insert_buffer -buffer_cell lib_cell
              [-net net]
              [-load_pin pin]
              [-load_pins list_of_pins]
              [-location {x y}]
              [-buffer_name name]
              [-net_name name]
              [-loads_on_same_net]
```

### Arguments

| Argument | Description |
| -------- | ----------- |
| `-buffer_cell` | Specified the library cell to use for the buffer. (Required) |
| `-location` | Specifies the `{x y}` coordinates (in microns) where the buffer should be placed. If not specified, the buffer is placed at the driver or load pin location. |
| `-buffer_name` | Specifies the base name for the new buffer instance. If not specified, "buf" is used. |
| `-net_name` | Specifies the base name for the new net created by the buffer insertion. If not specified, "net" is used. |
| `-net` | Specifies the net to be buffered. When used without `-load_pins`, it performs driver-side buffering. |
| `-load_pin` | Specifies a single load pin (input ITerm or BTerm) to buffer. |
| `-load_pins` | Specifies a list of load pins to buffer together. |
| `-loads_on_same_net` | A flag indicating that the specified `-load_pins` are on the same ODB net. If specified, multiple load pins on different flat nets can be buffered together and the target net among the multiple flat nets can be selected by `-net`. This option should be used with caution not to change the logical function of the design. |

---

## Buffering Cases

### 1. Driver-side Buffering (`-net`)
Inserts a buffer immediately after the driver pin of the specified net. The original net is connected to the buffer input, and a new net is created for the buffer output to drive all original loads.

**Example:**
```tcl
set net [get_nets u_mid1/u_leaf1/n1]
insert_buffer -net $net -buffer_cell BUF_X1 -buffer_name b_after_drvr
```

### 2. Single Load Buffering (`-load_pin`)
Inserts a buffer immediately before the specified load pin. A new net is created for the buffer output to drive only this specific load pin.

**Example:**
```tcl
set pin [get_pins u_mid1/u_leaf1/buf2/A]
insert_buffer -load_pin $pin -buffer_cell BUF_X1 -location {10.0 20.0}
```

### 3. Multiple Loads Buffering (`-load_pins` and optional `-net`)
Inserts a buffer that drives a specific subset of loads on a net. A new net is created for the buffer output to drive only the specified load pins. Other loads on the same net remain driven by the original driver.

**Example:**
```tcl
set net [get_nets u_mid1/l2_out1]
set loads [get_pins {u_mid1/dff_load1/D u_mid1/dff_load2/D}]
insert_buffer -net $net -load_pins $loads -buffer_cell BUF_X1
```

If `-net` is omitted, the command will attempt to infer the net from 
the first load pin.
`-net` may be required to specify the target net clearly when 
`-load_pins_on_same_net` is specified because the target load pins are not 
on the same net.

---

## C++ API Documentation

There are two levels of C++ APIs for buffer insertion: the high-level `Resizer` API and the low-level `dbNet` API.

### 1. `Resizer` API
The `Resizer` API is the recommended way to insert buffers as it handles higher-level tasks such as:
- Legalizing the buffer position (using `opendp`).
- Updating the design area.
- Updating the GUI.
- Incrementing the internal `inserted_buffer_count_`.
- Accepts STA objects (`sta::Net*`, `sta::PinSet`, etc.) which are easier to work with in the resizer context.

#### Methods in `Resizer` (rsz/Resizer.hh)

```cpp
// STA-based APIs
Instance* insertBufferAfterDriver(Net* net,
                                  LibertyCell* buffer_cell,
                                  const Point* loc = nullptr,
                                  const char* new_buf_base_name = nullptr,
                                  const char* new_net_base_name = nullptr);

Instance* insertBufferBeforeLoad(Pin* load_pin,
                                 LibertyCell* buffer_cell,
                                 const Point* loc = nullptr,
                                 const char* new_buf_base_name = nullptr,
                                 const char* new_net_base_name = nullptr);

Instance* insertBufferBeforeLoads(Net* net,
                                  const std::set<odb::dbObject*>& loads,
                                  LibertyCell* buffer_cell,
                                  const Point* loc = nullptr,
                                  const char* new_buf_base_name = nullptr,
                                  const char* new_net_base_name = nullptr,
                                  bool loads_on_same_db_net = true);

// ODB-based overloads
odb::dbInst* insertBufferAfterDriver(odb::dbNet* net,
                                     odb::dbMaster* buffer_cell,
                                     const Point* loc = nullptr,
                                     const char* new_buf_base_name = nullptr,
                                     const char* new_net_base_name = nullptr);

odb::dbInst* insertBufferBeforeLoad(odb::dbObject* load_pin,
                                    odb::dbMaster* buffer_cell,
                                    const Point* loc = nullptr,
                                    const char* new_buf_base_name = nullptr,
                                    const char* new_net_base_name = nullptr);

odb::dbInst* insertBufferBeforeLoads(odb::dbNet* net,
                                     const std::set<odb::dbObject*>& loads,
                                     odb::dbMaster* buffer_cell,
                                     const Point* loc = nullptr,
                                     const char* new_buf_base_name = nullptr,
                                     const char* new_net_base_name = nullptr,
                                     bool loads_on_same_db_net = true);
```

### 2. `dbNet` API
The `dbNet` API provides the core netlist transformation logic. It is a lower-level ODB API and does not perform legalization or GUI updates.

#### Methods in `dbNet` (odb/db.h)

```cpp
odb::dbInst* insertBufferAfterDriver(odb::dbObject* drvr_output_term,
                                     const odb::dbMaster* buffer_master,
                                     const odb::Point* loc = nullptr,
                                     const char* new_buf_base_name = nullptr,
                                     const char* new_net_base_name = nullptr,
                                     const odb::dbNameUniquifyType& uniquify = odb::dbNameUniquifyType::ALWAYS);

odb::dbInst* insertBufferBeforeLoad(odb::dbObject* load_input_term,
                                    const odb::dbMaster* buffer_master,
                                    const odb::Point* loc = nullptr,
                                    const char* new_buf_base_name = nullptr,
                                    const char* new_net_base_name = nullptr,
                                    const odb::dbNameUniquifyType& uniquify = odb::dbNameUniquifyType::ALWAYS);

odb::dbInst* insertBufferBeforeLoads(std::set<odb::dbObject*>& load_pins,
                                     const odb::dbMaster* buffer_master,
                                     const odb::Point* loc = nullptr,
                                     const char* new_buf_base_name = nullptr,
                                     const char* new_net_base_name = nullptr,
                                     const odb::dbNameUniquifyType& uniquify = odb::dbNameUniquifyType::ALWAYS,
                                     bool loads_on_same_db_net = true);
```

---

## Comparison: `Resizer` vs `dbNet` APIs

| Feature | `Resizer` API | `dbNet` API |
| ------- | ------------- | ----------- |
| **Object Types** | Uses STA objects (`Net*`, `Pin*`, `LibertyCell*`) | Uses ODB objects (`dbNet*`, `dbObject*`, `dbMaster*`) |
| **Legalization** | Automatically legalizes the buffer position. | No legalization. The buffer is placed exactly at `loc`. If `loc` is null, it is placed at the center (centroid) of the driver and load pins. |
| **Area Update** | Automatically updates the design area. | Does not update the design area. |
| **GUI Support** | Triggers GUI updates to show the new buffer. | No GUI integration. |
| **Usage** | Recommended. Best for interactive use and timing optimization flows. | Best for raw database manipulation where Resizer is not available. |

### C++ API Usage Examples

#### Using the `Resizer` API (High-Level)

```cpp
#include "rsz/Resizer.hh"

// 1. After Driver
rsz::Net* net = network->findNet("u1/n1");
rsz::LibertyCell* buffer_cell = resizer->selectBufferCell();
odb::Point loc(1000, 2000); // optional
rsz::Instance* buf = resizer->insertBufferAfterDriver(net, buffer_cell, &loc, "clkbuf", "clknet");

// 2. Before Load
rsz::Pin* load_pin = network->findPin("u2/A");
rsz::Instance* buf2 = resizer->insertBufferBeforeLoad(load_pin, buffer_cell);

// 3. Before Multiple Loads
std::set<odb::dbObject*> loads;
loads.insert(db_network->staToDb(network->findPin("u3/A")));
loads.insert(db_network->staToDb(network->findPin("u4/A")));
rsz::Instance* buf3 = resizer->insertBufferBeforeLoads(nullptr, loads, buffer_cell);
```

#### Using the `dbNet` API (Low-Level)

```cpp
#include "odb/db.h"

// 1. After Driver
odb::dbNet* db_net = block->findNet("n1");
odb::dbObject* drvr = db_net->getFirstDriverTerm();
odb::dbMaster* master = db->findMaster("BUF_X1");
odb::dbInst* buf = db_net->insertBufferAfterDriver(drvr, master, nullptr, "buf1");

// 2. Before Load
odb::dbITerm* iterm = inst->findITerm("A");
odb::dbInst* buf = db_net->insertBufferBeforeLoad(iterm, master);

// 3. Before Multiple Loads
std::set<odb::dbObject*> loads;
loads.insert(iterm1);
loads.insert(iterm2);
odb::dbInst* buf = db_net->insertBufferBeforeLoads(loads, master);
```

---

## Precautions

- **Legalization**: If you use the `dbNet` API, the buffer is placed at the specified location without checking for overlaps or core boundaries. If the location is not specified (`nullptr`), it is placed at the center (centroid) of the driver and load pins. Use `Resizer::insertBuffer*` if you need the buffer to be legally placed.
- **Hierarchical Flow**: The `insert_buffer` command fully supports hierarchical designs. When inserting buffers in a hierarchical context, the API handles the creation of hierarchical ports (`dbModBTerm`) and nets (`dbModNet`) as needed.
- **Inferred Nets**: When using `-load_pins` without `-net`, the tool attempts to infer the net from the first load pin. This works if all loads are on the same flat net.
- **Uniquify**: The API uses `dbNameUniquifyType::ALWAYS` by default to ensure that newly created instances and nets have unique names across the design hierarchy.

## Future Works

1. Support `-target_hierarchy` option to specify the hierarchy of the new buffer to be inserted.
2. Support the buffer placement on the existing physical route to minimize the routing impact.
3. Support inserting multiple buffers in series.
4. Support inserting inverter pair instead of buffer.
