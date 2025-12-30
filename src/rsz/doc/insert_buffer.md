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
              [-load_pins list_of_pins]
              [-location {x y}]
              [-buffer_name name]
              [-net_name name]
              [-load_pins_on_diff_nets]
```

### Arguments

| Argument | Description |
| -------- | ----------- |
| `-buffer_cell` | Specified the library cell to use for the buffer. (Required) |
| `-location` | Specifies the `{x y}` coordinates (in microns) where the buffer should be placed. If not specified, the buffer is placed at the driver or load pin location. |
| `-buffer_name` | Specifies the base name for the new buffer instance. If not specified, "buf" is used. Note that a unique suffix number will always be added to the base name to avoid a name collision. |
| `-net_name` | Specifies the base name for the new net created by the buffer insertion. If not specified, "net" is used. Note that a unique suffix number will always be added to the base name to avoid a name collision. |
| `-net` | Specifies the net to be buffered. When used without `-load_pins`, it performs driver-side buffering. |
| `-load_pins` | Specifies a single load pin or a list of load pins to buffer together. |
| `-load_pins_on_diff_nets` | A flag indicating that the specified `-load_pins` are on the different nets. If specified, multiple load pins on different flat nets can be buffered together and the target net among the multiple flat nets can be selected by `-net`. This option should be used with caution not to change the logical function of the design. |

---

## Buffering Cases

### 1. Driver-side Buffering (`-net`)
Inserts a buffer immediately after the driver pin of the specified net. The original net driving all loads is connected to the buffer output, and a new net is created for the buffer input to drive the new buffer instance.

**Example:**
```tcl
set net [get_nets u_mid1/u_leaf1/n1]
insert_buffer -net $net -buffer_cell BUF_X1 -buffer_name buf
```

### 2. Single Load Buffering (`-load_pins`)
Inserts a buffer immediately before the specified load pin. A new net is created for the buffer output to drive only this specific load pin.

**Example:**
```tcl
set pin [get_pins u_mid1/u_leaf1/buf2/A]
insert_buffer -load_pins $pin -buffer_cell BUF_X1 -location {10.0 20.0}
```

### 3. Multiple Loads Buffering (`-load_pins`)
Inserts a buffer that drives a specific subset of loads on a net. A new net is created for the buffer output to drive only the specified load pins. Other loads on the same net remain driven by the original driver.

**Example:**
```tcl
set loads [get_pins {u_mid1/dff_load1/D u_mid1/dff_load2/D}]
insert_buffer -load_pins $loads -buffer_cell BUF_X1
```

If `-net` is not specified, the command will attempt to infer the net 
from the first load pin.

### 4. Multiple Loads Buffering with loads on different nets (`-load_pins_on_diff_nets` and optional `-net`)

- **Load pins on the same net mode (Default)**: Omit `-load_pins_on_diff_nets`. 
The command expects all provided pins to be on the same flat net.
- **Load pins on different nets mode**: Specify `-load_pins_on_diff_nets` to 
allow buffering load pins from different `dbNet`s. With `-net` option, 
the target net on which the buffer is inserted can be selected.
If `-net` is not specified, the net connected to the first load pin will be 
selected as the target net.
Note that `-load_pins_on_diff_nets` and `-net` are meaningless 
when a single load pin is given by `-load_pins`.

**Example:**
```tcl
# Buffer pins across hierarchical boundaries
set loads [get_pins {u_load0/A h0/u_load1/B}]
set net [get_net -of_object [get_pins h0/u_load1/B]]
insert_buffer -net $net -load_pins $loads -buffer_cell BUF_X1 -load_pins_on_diff_nets
```

---

## C++ API Documentation

There are two levels of C++ APIs for buffer insertion: the high-level `Resizer` API and the low-level `dbNet` API.

### 1. `Resizer` API
The `Resizer` API is the recommended way to insert buffers as it handles higher-level tasks such as:
- Legalizing the buffer position (using `opendp`).
- Updating the design area.
- Updating the GUI.
- Incrementing the internal `Resizer::inserted_buffer_count_`.
- Accepts STA objects (`sta::Net*`, `sta::PinSet`, etc.) which are easier to work with in the resizer context.

#### Methods in `Resizer` (rsz/Resizer.hh)

```cpp
// STA-based APIs
Instance* insertBufferAfterDriver(Net* net,
                                  LibertyCell* buffer_cell,
                                  const Point* loc = nullptr,
                                  const char* new_buf_base_name = nullptr,
                                  const char* new_net_base_name = nullptr,
                                  const odb::dbNameUniquifyType& uniquify
                                  = odb::dbNameUniquifyType::ALWAYS);

Instance* insertBufferBeforeLoad(Pin* load_pin,
                                 LibertyCell* buffer_cell,
                                 const Point* loc = nullptr,
                                 const char* new_buf_base_name = nullptr,
                                 const char* new_net_base_name = nullptr,
                                 const odb::dbNameUniquifyType& uniquify
                                 = odb::dbNameUniquifyType::ALWAYS);

// PinSeq* overload
Instance* insertBufferBeforeLoads(Net* net,
                                  PinSeq* loads,
                                  LibertyCell* buffer_cell,
                                  const Point* loc = nullptr,
                                  const char* new_buf_base_name = nullptr,
                                  const char* new_net_base_name = nullptr,
                                  const odb::dbNameUniquifyType& uniquify
                                  = odb::dbNameUniquifyType::ALWAYS,
                                  bool loads_on_diff_nets = false);

// PinSet* overload
Instance* insertBufferBeforeLoads(Net* net,
                                  PinSet* loads,
                                  LibertyCell* buffer_cell,
                                  const Point* loc = nullptr,
                                  const char* new_buf_base_name = nullptr,
                                  const char* new_net_base_name = nullptr,
                                  const odb::dbNameUniquifyType& uniquify
                                  = odb::dbNameUniquifyType::ALWAYS,
                                  bool loads_on_diff_nets = false);

// ODB-based overloads
odb::dbInst* insertBufferAfterDriver(odb::dbNet* net,
                                     odb::dbMaster* buffer_cell,
                                     const Point* loc = nullptr,
                                     const char* new_buf_base_name = nullptr,
                                     const char* new_net_base_name = nullptr,
                                     const odb::dbNameUniquifyType& uniquify
                                     = odb::dbNameUniquifyType::ALWAYS);

odb::dbInst* insertBufferBeforeLoad(odb::dbObject* load_pin,
                                    odb::dbMaster* buffer_cell,
                                    const Point* loc = nullptr,
                                    const char* new_buf_base_name = nullptr,
                                    const char* new_net_base_name = nullptr,
                                    const odb::dbNameUniquifyType& uniquify
                                    = odb::dbNameUniquifyType::ALWAYS);

odb::dbInst* insertBufferBeforeLoads(odb::dbNet* net,
                                     const std::set<odb::dbObject*>& loads,
                                     odb::dbMaster* buffer_cell,
                                     const Point* loc = nullptr,
                                     const char* new_buf_base_name = nullptr,
                                     const char* new_net_base_name = nullptr,
                                     const odb::dbNameUniquifyType& uniquify
                                     = odb::dbNameUniquifyType::ALWAYS,
                                     bool loads_on_diff_nets = false);
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
                                     const odb::dbNameUniquifyType& uniquify
                                     = odb::dbNameUniquifyType::ALWAYS);

odb::dbInst* insertBufferBeforeLoad(odb::dbObject* load_input_term,
                                    const odb::dbMaster* buffer_master,
                                    const odb::Point* loc = nullptr,
                                    const char* new_buf_base_name = nullptr,
                                    const char* new_net_base_name = nullptr,
                                    const odb::dbNameUniquifyType& uniquify
                                    = odb::dbNameUniquifyType::ALWAYS);

// std::set overload
odb::dbInst* insertBufferBeforeLoads(std::set<odb::dbObject*>& load_pins,
                                     const odb::dbMaster* buffer_master,
                                     const odb::Point* loc = nullptr,
                                     const char* new_buf_base_name = nullptr,
                                     const char* new_net_base_name = nullptr,
                                     const odb::dbNameUniquifyType& uniquify
                                     = odb::dbNameUniquifyType::ALWAYS,
                                     bool loads_on_diff_nets = false);

// std::vector overload
odb::dbInst* insertBufferBeforeLoads(std::vector<odb::dbObject*>& load_pins,
                                     const odb::dbMaster* buffer_master,
                                     const odb::Point* loc = nullptr,
                                     const char* new_buf_base_name = nullptr,
                                     const char* new_net_base_name = nullptr,
                                     const odb::dbNameUniquifyType& uniquify
                                     = odb::dbNameUniquifyType::ALWAYS,
                                     bool loads_on_diff_nets = false);
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

#### Using the `Resizer` API (High-level)

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
rsz::PinSet loads(network);
loads.insert(network->findPin("u3/A"));
loads.insert(network->findPin("u4/A"));
rsz::Instance* buf3 = resizer->insertBufferBeforeLoads(nullptr, &loads, buffer_cell);
```

#### Using the `dbNet` API (Low-level)

```cpp
#include "odb/db.h"

// 1. After Driver
odb::dbNet* db_net = block->findNet("n1");
odb::dbObject* drvr = db_net->getFirstDriverTerm();
odb::dbMaster* master = db->findMaster("BUF_X1");
odb::dbInst* buf = db_net->insertBufferAfterDriver(drvr, master, nullptr, "buf1");

// 2. Before Load
odb::dbInst* inst = db->findInst("u2");
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
