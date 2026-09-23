# check_macro_placement without a design is an error (MPL-0080), and when
# the movable cells do not fit in the macro placement area it fails with the
# error rtl_macro_placer gives for that (MPL-0065).
source "helpers.tcl"

set failed [catch { check_macro_placement } message]
check "no design: the check fails" { set failed } 1
check "no design: MPL-0080" { string match {*MPL-0080*} $message } 1

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/macro_only.lef"

read_def "./testcases/unfixed_cells_dont_fit_in_core.def"

set_thread_count 0
set failed [catch { check_macro_placement } message]
check "cells do not fit: the check fails" { set failed } 1
check "cells do not fit: MPL-0065" { string match {*MPL-0065*} $message } 1

set failed [catch { rtl_macro_placer -report_directory [make_result_dir] } message]
check "cells do not fit: rtl_macro_placer fails the same way" \
  { expr { $failed && [string match {*MPL-0065*} $message] } } 1

exit_summary
