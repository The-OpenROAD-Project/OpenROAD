# OpenROAD

[![Build Status](https://jenkins.openroad.tools/buildStatus/icon?job=OpenROAD-Public%2Fmaster)](https://jenkins.openroad.tools/job/OpenROAD-Public/job/master/)
[![Coverity Scan Status](https://scan.coverity.com/projects/the-openroad-project-openroad/badge.svg)](https://scan.coverity.com/projects/the-openroad-project-openroad)
[![Documentation Status](https://readthedocs.org/projects/openroad/badge/?version=latest)](https://openroad.readthedocs.io/en/latest/?badge=latest)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/5370/badge)](https://bestpractices.coreinfrastructure.org/en/projects/5370)

## About OpenROAD

OpenROAD is the leading open-source, foundational application for
semiconductor digital design. The OpenROAD flow delivers an
Autonomous, No-Human-In-Loop (NHIL) flow, 24 hour turnaround from
RTL-GDSII for rapid design exploration and physical design implementation.

```mermaid
:align: center
%%{
  init: {
    'theme': 'neutral',
    'themeVariables': {
      'textColor': '#000000',
      'noteTextColor' : '#000000',
      'fontSize': '20px'
    }
  }
}%%
flowchart TB
    A[Verilog\n+ libraries\n + constraints] --> FLOW
    style A fill:#74c2b5
    subgraph FLOW
    style FLOW fill:#FFFFFF00,stroke-width:4px

    direction TB
        B[Logic Synthesis]
        B --> C[Floorplanning] 
        C --> D[Placement & Optimization]
        D --> E[CTS & Optimization]
        E --> F[Global & Detailed Routing]
        F --> G[Layout Finishing]
        style B fill:#f8cecc,stroke:#000000,stroke-width:4px
        style C fill:#fff2cc,stroke:#000000,stroke-width:4px
        style D fill:#cce5ff,stroke:#000000,stroke-width:4px
        style E fill:#67ab9f,stroke:#000000,stroke-width:4px
        style F fill:#fa6800,stroke:#000000,stroke-width:4px
        style G fill:#ff6666,stroke:#000000,stroke-width:4px
    end

    FLOW --> H[GDSII\n Final Layout]
    style H fill:#ff0000,stroke:#000000,stroke-width:4px
```

Documentation is also available [here](https://openroad.readthedocs.io/en/latest/main/README.html).

## OpenROAD Mission

[OpenROAD](https://theopenroadproject.org/) eliminates the barriers
of cost, schedule risk and uncertainty in hardware design to promote
open access to rapid, low-cost IC design software and expertise and
system innovation. The OpenROAD application enables flexible flow
control through an API with bindings in Tcl and Python.

OpenROAD is used in research and commercial applications such as,
- [OpenROAD-flow-scripts](https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts)
  from [OpenROAD](https://theopenroadproject.org/)
- [OpenLane](https://github.com/The-OpenROAD-Project/OpenLane) from
  [Efabless](https://efabless.com/)
- [Silicon Compiler](https://github.com/siliconcompiler/siliconcompiler)
  from [Zero ASIC](https://www.zeroasic.com/)
- [Hammer](https://docs.hammer-eda.org/en/latest/Examples/openroad-nangate45.html)
  from [UC Berkeley](https://github.com/ucb-bar)
- [OpenFASoC](https://github.com/idea-fasoc/OpenFASOC) from
  [IDEA-FASoC](https://github.com/idea-fasoc) for mixed-signal design flows

OpenROAD fosters a vibrant ecosystem of users through active
collaboration and partnership through software development and key
alliances. Our growing user community includes hardware designers,
software engineers, industry collaborators, VLSI enthusiasts,
students and researchers.

OpenROAD strongly advocates and enables IC design-based education
and workforce development initiatives through training content and
courses across several global universities, the Google-SkyWater
[shuttles](https://platform.efabless.com/projects/public) also
includes GlobalFoundries shuttles, design contests and IC design
workshops. The OpenROAD flow has been successfully used to date
in over 600 silicon-ready tapeouts for technologies up to 12nm.

## Getting Started with OpenROAD-flow-scripts

OpenROAD provides [OpenROAD-flow-scripts](https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts)
as a native, ready-to-use prototyping and tapeout flow. However,
it also enables the creation of any custom flow controllers based
on the underlying tools, database and analysis engines. Please refer to the flow documentation [here](https://openroad-flow-scripts.readthedocs.io/en/latest/).

OpenROAD-flow-scripts (ORFS) is a fully autonomous, RTL-GDSII flow
for rapid architecture and design space exploration, early prediction
of QoR and detailed physical design implementation. However, ORFS
also enables manual intervention for finer user control of individual
flow stages through Tcl commands and Python APIs.

Figure below shows the main stages of the OpenROAD-flow-scripts:

```mermaid
:align: center
%%{
  init: {
    'theme': 'neutral',
    'themeVariables': {
      'textColor': '#000000',
      'noteTextColor' : '#000000',
      'fontSize': '20px'
    }
  }
}%%

flowchart TB
    A[RTL-GDSII\n Using\n OpenROAD-flow-scripts] --> SYNTHESIS
    style A stroke:#000000,stroke-width:4px
    SYNTHESIS --> FLOORPLAN; 
    %% FLOORPLAN --> PLACEMENT; PLACEMENT --> CTS
    %%CTS --> ROUTING; ROUTING -->FINISH
    subgraph SYNTHESIS
    style SYNTHESIS fill:#f8cecc,stroke:#000,stroke-width:4px
    direction TB
    RTL & SDC & .lib & SYNTHESIS_2(Mapping files, etc..) --> SYNTHESIS_1(Yosys)
    SYNTHESIS_1 --> Netlist & SYNTHESIS_5(SDC)
    %%B1(Tool:Yosys);  B2(Input files);  B3(Output files)
    %%B2 --> RTL & SDC & .lib & B4(Mapping files, etc..)
    %%B3 --> Netlist & B5(SDC)
    end
    subgraph FLOORPLAN
    direction TB
    style FLOORPLAN fill:#fff2cc,stroke:#000000,stroke-width:4px
    C1("Import all necessary files\n (Netlist, SDC, etc...) and
            check initial timing report")
    C1 --> C2("Translate .v to .odb/\n Floorplan Initialization")
    C2 --> C3("IO placement (random)")
    C3 --> C4("Automatic partitioning")
    C4 --> C5("Timing-driven mixed-size placement")
    C5 --> C6("Macro placement/Hier-RTLMP")
    C6 --> C7("Tapcell and well tie insertion")
    C7 --> C8("PDN generation")
    C8 --> C9("Floorplan .odb and .sdc file generation")
    end

    FLOORPLAN--> PLACEMENT; PLACEMENT --> CTS; CTS --> ROUTING; ROUTING -->FINISH
    subgraph PLACEMENT
    direction TB
    style PLACEMENT fill:#cce5ff,stroke:#000000,stroke-width:4px
    D1("Global placement without placed IOs
                (Timing and routability-driven)")
    D1 --> D2("IO placement (non-random)")
    D2 --> D3("Global placement with placed IOs
                (Timing and routability-driven)")
    D3 --> D4("Resizing and buffering")
    D4 --> D5("Detailed placement")
    end
    subgraph CTS
    direction TB
    style CTS fill:#67ab9f,stroke:#000000,stroke-width:4px
    E1("Clock Tree synthesis")
    E1 --> E2("Timing optimization")
    E2 --> E3("Filler cell insertion")
    end
    subgraph ROUTING
    direction TB
    style ROUTING fill:#fa6800,stroke:#000000,stroke-width:4px
    F1("Global routing")
    F1 --> F2("Detailed routing")
    end
    subgraph FINISH
    direction TB
    style FINISH fill:#ff6666,stroke:#000000,stroke-width:4px
    G1("Generate GDSII Tool:\n KLayout")
    G1 --> G2("Metal Fill Insertion")
    G2 --> G3("Signoff timing report")
    G3 --> G4("DRC/LVS check\n Tool: Klayout")
    end
```

Here are the main steps for a physical design implementation
using OpenROAD;

- `Floorplanning`
  - Floorplan initialization - define the chip area, utilization
  - IO pin placement (for designs without pads)
  - Tap cell and well tie insertion
  - PDN- power distribution network creation
- `Global Placement` - Minimize wirelengths
  - Macro placement (RAMs, embedded macros)
  - Standard cell placement
  - Automatic placement optimization and repair for max slew,
    max capacitance, and max fanout violations and long wires
- `Detailed Placement`
  - Legalize placement - align to grid, adhere to design rules
  - Incremental timing analysis for early estimates
- `Clock Tree Synthesis` - Generate a balanced tree to meet timing
  and reduce skews
  - Insert buffers and resize for high fanout nets
- `Optimize setup/hold timing`
- `Global routing`
  - Antenna repair
  - Create routing guides
- `Detailed routing`
  - Legalize routes, DRC-correct routing to meet timing, power
    constraints
- `Chip Finishing`
  - Parasitic extraction using OpenRCX
  - Final timing verification
  - Final physical verification
  - Dummy metal fill for manufacturability
  - Use KLayout or Magic using generated GDS for DRC signoff

### GUI

The OpenROAD GUI is a powerful visualization, analysis, and debugging
tool with a customizable Tcl interface. The below figures show GUI views for
various flow stages including floorplanning, placement congestion,
CTS and post-routed design.

#### Floorplan

![ibex_floorplan.webp](./docs/images/ibex_floorplan.webp)

#### Automatic Hierarchical Macro Placement

![Ariane133](./docs/images/ariane133_mpl2.webp)

#### Placement Congestion Visualization

![pl_congestion.webp](./docs/images/pl_congestion.webp)

#### CTS

![clk_routing.webp](./docs/images/clk_routing.webp)

#### Routing

![ibex_routing.webp](./docs/images/ibex_routing.webp)

### PDK Support

The OpenROAD application is PDK independent. However, it has been tested
and validated with specific PDKs in the context of various flow
controllers.

OpenLane supports SkyWater 130nm and GlobalFoundries 180nm.

OpenROAD-flow-scripts supports several public and private PDKs
including:

#### Open-Source PDKs

-   `GF180` - 180nm
-   `SKY130` - 130nm
-   `Nangate45` - 45nm
-   `ASAP7` - Predictive FinFET 7nm

#### Proprietary PDKs

These PDKS are supported in OpenROAD-flow-scripts only. They are used to
test and calibrate OpenROAD against commercial platforms and ensure good
QoR. The PDKs and platform-specific files for these kits cannot be
provided due to NDA restrictions. However, if you are able to access
these platforms independently, you can create the necessary
platform-specific files yourself.

-   `GF55` - 55nm
-   `GF12` - 12nm
-   `Intel22` - 22nm
-   `Intel16` - 16nm
-   `TSMC65` - 65nm

## Tapeouts

OpenROAD has been used for full physical implementation in over
600 tapeouts in SKY130 and GF180 through the Google-sponsored,
Efabless [MPW shuttle](https://efabless.com/open_shuttle_program)
and [ChipIgnite](https://efabless.com/) programs.

![shuttle.webp](./docs/images/shuttle.webp)

### OpenTitan SoC on GF12LP - Physical design and optimization using OpenROAD

![OpenTitan_SoC.webp](./docs/images/OpenTitan_SoC.webp)

### Continuous Tapeout Integration into CI

The OpenROAD project actively adds successfully taped out MPW shuttle
designs to the [CI regression
testing](https://github.com/The-OpenROAD-Project/OpenLane-MPW-CI).
Examples of designs include Open processor cores, RISC-V based SoCs,
cryptocurrency miners, robotic app processors, amateur satellite radio
transceivers, OpenPower-based Microwatt etc.

## Build OpenROAD

To build OpenROAD tools locally in your machine, follow steps
from [here](docs/user/Build.md).

## Regression Tests

There are a set of regression tests in `test/`.

``` shell
# run all tool unit tests
test/regression
# run all flow tests
test/regression flow
# run <tool> tests
test/regression <tool>
# run <tool> tool tests
src/<tool>/test/regression
```

The flow tests check results such as worst slack against reference values.
Use `report_flow_metrics [test]...` to see all of the metrics.
Use `save_flow_metrics [test]...` to add margins to the metrics and save them to <test>.metrics_limits.

``` text
% report_flow_metrics gcd_nangate45
                       insts    area util slack_min slack_max  tns_max clk_skew max_slew max_cap max_fanout DPL ANT drv
gcd_nangate45            368     564  8.8     0.112    -0.015     -0.1    0.004        0       0          0   0   0   0
```

## Run

``` text
openroad [-help] [-version] [-no_init] [-exit] [-gui]
         [-threads count|max] [-log file_name] cmd_file
  -help              show help and exit
  -version           show version and exit
  -no_init           do not read .openroad init file
  -threads count|max use count threads
  -no_splash         do not show the license splash at startup
  -exit              exit after reading cmd_file
  -gui               start in gui mode
  -python            start with python interpreter [limited to db operations]
  -log <file_name>   write a log in <file_name>
  cmd_file           source cmd_file
```

OpenROAD sources the Tcl command file `~/.openroad` unless the command
line option `-no_init` is specified.

OpenROAD then sources the command file `cmd_file` if it is specified on
the command line. Unless the `-exit` command line flag is specified, it
enters an interactive Tcl command interpreter.

A list of the available tools/modules included in the OpenROAD app
and their descriptions are available [here](docs/contrib/Logger.md#openroad-tool-list).

## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.
