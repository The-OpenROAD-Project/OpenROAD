# TritonMacroPlacer

ParquetFP based macro cell placer for OpenROAD.

## Flows
* Input: Initial placed DEF of a mixed-size (macros + cells) netlist. Such a DEF is produced by RePlAce (timing-driven, mixed-size mode, aka "TD-MS-RePlAce").
* Output: A placed DEF with macro placements honoring halos, channels and cell row "snapping".  Approximately ceil((#macros/3)^(3/2)) "sets" corresponding to quadrisections of the initial placed mixed-size DEF are explored and packed using ParquetFP-based annealing. The best resulting floorplan according to a heuristic evaluation function is returned.

## How to Download and Build?
- If you want to use this as part of the OpenROAD project you should build it and use it from inside the integrated [OpenROAD app](https://github.com/The-OpenROAD-Project/OpenROAD). 
- For OpenROAD-flow users, manuals for released binaries are available in readthedocs. [(Getting-Started)](https://openroad.readthedocs.io/en/latest/user/getting-started.html)
- For developers, manuals for building a binary is available in OpenROAD app repo. [(OpenROAD app)](https://github.com/The-OpenROAD-Project/OpenROAD) 
- Note that TritonMacroPlace is a submodule of OpenROAD repo, and take a place as the **"macro_placement"** command. 


## OpenROAD Tcl Usage (macro_placement)

```
macro_placement -global_config <global_config_file>
```
* __global_config__ : Set global config file loction. [string]

### Global Config Example
```
set ::HALO_WIDTH_V 1
set ::HALO_WIDTH_H 1
set ::CHANNEL_WIDTH_V 0
set ::CHANNEL_WIDTH_H 0
```
* __HALO_WIDTH_V__ : Set macro's vertical halo. [float; unit: micron]
* __HALO_WIDTH_H__ : Set macro's horizontal halo. [float; unit: micron]
* __CHANNEL_WIDTH_V__ : Set macro's vertical channel width. [float; unit: micron]
* __CHANNEL_WIDTH_H__ : Set macro's horizontal channel width. [float; unit: micron]


## License
* BSD-3-clause License [[Link]](LICENSE)
* Code found under the Modules directory (e.g., submodules ParquetFP and ABKCommon source files) have individual copyright and license declarations at the top of each file.  

## 3rd Party Module List
* ParquetFP from UMPack
* ABKCommon from UMPack
