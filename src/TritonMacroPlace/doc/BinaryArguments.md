# Usage

    $ ./fplan -lef tech.lef -lef macro.lef ...  -def <*.def> -verilog <*.v> -lib lib1.lib -lib lib2.lib ... -sdc <*.sdc> -design <designName> -output <outputLocation> -depth 3 -globalConfig global.cfg -localConfig local.cfg [Options]

* __-lef__ : \*.lef Location (Multiple lef files supported. __Technology LEF must be ahead of other LEFs.__)
* __-def__ : \*.def Location (Required due to FloorPlan information)
* __-verilog__ : \*.v Location (Optional)
* __-design__ : Specify the top-module design name. (Required for OpenSTA). 
* __-sdc__ : Specify the Synopsys Design Constraint (SDC) file. (Required for OpenSTA)
* __-lib__ : \*.lib Location (Multiple lib files supported. Required for OpenSTA)
* __-output__ : Specify the Location of Output Results (TritonMacroPlacer will generate multiple def files)
* __-depth__ : Specify the BFS search depth from a sequential graph. Default is 3.
* __-globalConfig__ : Specify the Location of IP_global.cfg file. A description is in [here](IP_global.md)
* __-localConfig__ : Specify the Location of IP_local.cfg file. A description is in [here](IP_local.md) (Optional)
* __-plot__ : Plot the results as gnuplot formats.
* __-generateAll__ : Generate all of possible solutions. 
* __-randomPlace__ : Place macros randomly before running TritonMacroPlace (not recommended). 
 
