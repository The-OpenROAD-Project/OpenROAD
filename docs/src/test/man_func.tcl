source "helpers.tcl"
# Tcl man functionality test

# Test 1) Running man with -manpath set to the generated pages in docs/cat.
set manpath_dir [file join [pwd] .. .. cat]
man openroad -manpath $manpath_dir -no_pager

# Test 2) Running man with no -manpath. A test binary has no installed
# pages, so this reports that the page is not found.
catch { man openroad -no_pager } err
puts $err
