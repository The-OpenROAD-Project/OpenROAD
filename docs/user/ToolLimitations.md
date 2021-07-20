# Capabilities/Limitations

## Global Considerations

- OpenROAD v1.0 production will be focused on the tapeout mentioned in
    the above introduction. Features will be implemented in priority
    order based on our sponsor requirement to make the chosen design
    manufacturable. In Phase 2 of the IDEA program, the OpenROAD tool
    feature set will be rounded out and more of the project's flow and
    tool research objectives will be addressed.
- Each new design enablement (foundry process/PDK, library, IPs) will
    require setup via configuration files, one-time characterizations,
    etc. as documented with the tool. Examples include (i) the setup of
    PDN generation, (ii) the creation of "wrapped LEF abstracts" for
    cells and/or macros to comply with Generic Node Enablement (see
    Routing, below), and (iii) the creation of characterized lookup
    tables to guide CTS buffering.

## Supported Platforms

- OpenROAD v1.0 will build on "bare metal", CentOS 7 with required
    packages installed as specified in the README.
- MacOS will also be supported.
- Users with access to Docker will also be able to build on any
    machine using the included Dockerfile.

## Design Partitioning and Logic Synthesis

- Logic Synthesis (Yosys) will accept only hierarchical RTL Verilog.
- SystemVerilog to Verilog conversion must be performed by the user
    (e.g., using bsg sv2v or any tool of their choosing) before running
    Yosys.
- Logic Synthesis is one of potentially multiple steps in OpenROAD
    that may require a single merged LEF as of the v1.0 release. A
    utility script to perform merging is
    [here](https://github.com/The-OpenROAD-Project/alpha-release/blob/master/flow/scripts/mergeLib.pl).
- To support convergence in the downstream place-CTS-route steps, it
    is advisable to exclude cells that risk difficult pin access (e.g.,
    sub-X1 sizes) and/or to invoke cell padding during placement. The
    cell exclusion would be akin to a "dont_use" list, which is not
    currently supported and must be manually implemented by editing the
    library files.

## STA

- Supports multi-corner analysis (e.g., setup and hold), but with
    limit of one mode.
- SDC support up to latest public, open version (e.g., SDC 1.4).
- No SI analysis: any coupling caps can be multiplied by a "Miller
    Coupling Factor" (MCF) and then treated as grounded.
- No CCS/ECSM (current-source model) support.
- No LVF support.
- No PBA analysis option.
- No instance IR drop (i.e., setting a rail voltage for given
    instance).
- No reduction of non-tree wiring topologies. (Arnoldi reduction
    provided along with O'Brien-Savarino, 2-pole, Elmore reduction and
    delay calculation options.)

## Floorplan

- Macro placement is limited to 100 RAMs/macros per P&R block.
- PDN configuration files must be provided by the user. These are
    documented in the "pdngen" tool repo,
    [here](https://github.com/The-OpenROAD-Project/OpenROAD/blob/master/src/pdn/README.md).

## Placement

- A P&R block is limited to one logic power domain and one I/O power
    domain. Additional power domains must be handled manually (OpenROAD
    Tcl scripting).
- Isolation cells, level converters and power management must be
    manually inserted into the layout by the user (e.g., as
    pre-placements).
- No support of UPF/CPF formats for power intent.
- Support of user guidance for logic clustering and placement will be
    limited to "fence" and "pre-placement" guidance, with the caveat
    that such guidance may degrade solution QOR in the OpenROAD flow.

## Clock Tree Synthesis

- Support only positive edge-triggered FFs
- Hold buffering will be at post-CTS and not later in the flow

## Routing

- The TritonRoute router will not understand LEF57, LEF58 constructs
    in techlef: the workaround is OpenROAD Generic Node Enablement (see
    "OpenROAD Requirements for Generic Node Enablement, at [this
    link](https://docs.google.com/document/d/1-KyRNu7qU_7oMYxXB5ToTkLv2C9AJbUAHJQr24rIU7U/edit?ts=5db1f0b2)).
- Users should be advised that TritonRoute does not handle coloring
    explicitly; a color-correct-by-construction methodology (e.g., for
    Mx layers in 14/12nm) is achieved via Generic Node Enablement.
- Antenna checking and fixing capability is committed for v1.0.

## Layout Finishing and Final Verifications

- Parasitic extraction (SPEF from layout) is unlikely to comprehend
    coupling.
- There is no "signoff-quality electrical/performance analysis"
    counterpart to "PrimeTime-SI" (timing, signal integrity) or
    "Voltus"/"RedHawk" (power integrity).
- A golden PV tool will be the evaluator for DRC.
- Generation of merged GDS currently requires a Magic 8.2 tech file.
    Details are given
    [here](https://github.com/The-OpenROAD-Project-Attic/OpenROAD-Utilities/tree/master/def-to-gdsii).
- Export of merged GDS does not add text markings that may be expected
    by commercial physical verification tools.
- For supported design tape-outs (particularly, at a commercial
    14/12nm node, up through July 2020), physical verification (DRC/LVS)
    is expected to be performed by the design team using commercial
    tools. (Everything up to routed DEF and merged GDS will be produced
    by OpenROAD or other open-source tools.)
