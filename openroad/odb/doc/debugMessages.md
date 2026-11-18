# OpenDB Debug Messages

## Groups

### DB editing
- Group Name: `DB_EDIT`
- Levels:
1. Netlist editing operations (e.g., connect, disconnect, create, ...).
2. Other operations. (e.g., create dbGuide, add resistance segment, adjust cap node, field value update, ...).

### ECO journal
- Group Name: `DB_ECO`
- Levels:
1. Certain operations (e.g., create dbCCSeg)
2. Most ECO operations
3. Undo ECO operations + dbGuide creation
4. Undo ECO status change (e.g., start of undo, early exit of undo, ...)

### Replace design
- Group Name: `replace_design`
- Levels:
1. Module/Instance/Port creation and connection operations.
2. Print iterms in a new module during swap master operation.
3. Skipping non-internal nets during replacement.

### Verilog Reader
- Group Name: `dbReadVerilog`
- Levels:
1. High-level hierarchy creation (Blocks, Modules, Instances, Ports), Properties, Linking, and dumping block content.
2. Detailed netlist creation (Nets, Connections, Child instances).

### Get Default Vias
- Group Name: `get_default_vias`
- Levels:
1. Default via resolution process (candidates, selection, or missing defaults).
