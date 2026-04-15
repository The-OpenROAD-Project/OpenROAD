# Regression test for issue #9932: DPL-0400 topological sort cycle
# in shift legalizer with multi-height cells.
read_lef $::env(MULTI_HEIGHT_LEF)
read_def $::env(MULTI_HEIGHT_DEF)
improve_placement
