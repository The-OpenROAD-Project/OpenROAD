Metrics
=======

The `OpenROAD-flow-scripts`_ repository contains source files (e.g., LEF/DEF,
verilog, SDC, Liberty, RC) and configuration files (e.g. `config.mk`)
that enable the user to run a small set of designs through our complete
RTL-GDS flow.

To keep track of the quality of the results, we maintain inside each design
folder two files:

1. `metadata-base-ok.json` which contains all the relevant information
extracted from the "golden" execution of the flow (i.e., last known good
result).

2. `rules.json` which holds a set of rules that we use to evaluate new
executions when a change is made.


The evaluation checks for key metrics (e.g., worst slack, number of DRCs)
to ensure that changes do not degrade too much w.r.t. the "golden" values.

Checking against golden
-----------------------

After you make a significant change, e.g., fixed a bug in a piece of code,
or changed some configuration variable (`PLACE_DENSITY`), you should review
the results and compare them with the "golden". To perform the check,
you will need to run the following command:

.. code-block:: shell

      # the clean_metadata is only required if you need to re-run
      make [clean_metadata] metadata

If the command above yields any error message, please review them to make
sure the change in metrics is expected and justifiable. If so, proceed to
the next section to update the "golden" reference.

Update process
--------------

The update of the reference files is mandatory if the metrics got worse
than the values limited by the `rules.json` (see previous section on how
to perform the check). Also it is a good idea to update the "golden" files
if your changed improved a metric to ensure that the improvement will not
be lost in the future.

To update all the reference files:

.. code-block:: shell

      make update_ok

In case you have a special reason to only update one of the files, you can
do so with the following commands:

.. code-block:: shell

      # update metadata-base-ok.json file for the design
      make update_metadata

      # update rules.json file for the design
      # this will use the current (+ a padding) metadata-base-ok.json
      # the padding ensures that small changes do not break the flow
      make update_rules


.. _`OpenROAD-flow-scripts`: https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts
