Metrics
=======

The `OpenROAD-flow-scripts`_ repository contains source files (e.g., LEF/DEF,
verilog, SDC, Liberty, RC) and configuration files (e.g. `config.mk`)
that enable the user to run a small set of designs through our complete
RTL-GDS flow. To keep track of the quality of the results, we maintain
inside each design folder two files: (i) `metadata-base-ok.json` and (ii)
`rules.json`. The file (i) contains all the relevant information extracted
from the "golden" execution of the flow (i.e., last known good result). On
the other hand file (ii) holds a set of rules that we use to evaluate new
executions when a change is made.  The evaluation checks for key metrics
(e.g., worst slack, number of DRCs) to ensure that changes do not degrade
too much w.r.t. the "golden" values.

Checking against golden
-----------------------

..code-block:: sh

      make [clean_metadata] metadata

Update process
--------------

If you made a significant change, e.g., fixed a bug in a piece of code,
or changed some configuration variable (`PLACE_DENSITY`), you should review
the results (using the commands from the previous section). The update of
the reference files will be mandatory if the metrics got worse than the
values from the `rules.json`, also it is a good idea to update the "golden"
files if your changed improved a metric to ensure that the improvement will
not be lost in the future.

..code-block:: sh

      # update all reference files [RECOMMENDED]
      make update_ok

      # update metadata-base-ok.json file for the design
      make update_metadata

      # update rules.json file for the design
      # this will use the current (+ a padding) metadata-base-ok.json
      # the padding ensures that small changes do not break the flow
      make update_rules


.. _`OpenROAD-flow-scripts`: https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts
