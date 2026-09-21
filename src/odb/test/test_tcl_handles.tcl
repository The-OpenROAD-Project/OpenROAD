source "helper.tcl"

lassign [createSimpleDB] db lib
set block [create1LevelBlock $db $lib [$db getChip]]

# Borrowed handles are plain strings, so walking a design registers no Tcl
# command per object.
foreach net [$block getNets] {
  foreach iterm [$net getITerms] {
    $iterm getNet
  }
}
foreach inst [$block getInsts] {
  $inst getMaster
}
set handle_cmds [llength [info commands _*_p_odb__*]]
assert {$handle_cmds == 0} \
  "walking the block left $handle_cmds handle commands registered"

# Methods still dispatch on a handle with no command of its own, including ones
# inherited from a base class and SWIG's own attribute accessors.
set net [lindex [$block getNets] 0]
assert {[llength [info commands $net]] == 0} "handle $net is a command"
assertStringNotEq [$net getName] "" "getName failed on a borrowed handle"
assert {[$net getId] > 0} "inherited getId failed on a borrowed handle"
assertStringEq [$net cget -this] $net "cget -this did not round trip"

# The same handle spelled with the global namespace qualifier, both as a command
# and as an argument.
assertStringEq [::$net getName] [$net getName] "::\$handle dispatch failed"
assertStringEq [odb::dbNet_getName ::$net] [$net getName] \
  "::\$handle failed as an argument"

# A handle names the same object every time it is fetched.
assertStringEq [lindex [$block getNets] 0] $net "handle identity is not stable"

# Objects SWIG owns keep a command of their own, which is what destroys them.
odb::Rect rect 100 200 300 400
assert {[llength [info commands rect]] == 1} "named constructor made no command"

# An object command passed by name still converts, qualified or not, and from
# inside another namespace.  Object commands live in the global namespace, so
# both spellings name the same object.
assert {[odb::Rect_xMin rect] == 100} "rect did not convert by name"
assert {[odb::Rect_xMin ::rect] == 100} "::rect did not convert by name"
namespace eval elsewhere {
assert {[odb::Rect_xMin ::rect] == 100} "::rect did not convert from a namespace"
}
rename rect {}

# A command that is not a handle still errors the usual way.
assert {[catch { no_such_command_at_all }] == 1} "unknown command did not error"

# Commands that are not handles reach the handler odb_unknown displaced, even
# when an application installs one of its own after odb is initialized.
proc app_unknown { args } { return "app_unknown saw [lindex $args 0]" }
uplevel #0 [list namespace unknown app_unknown]
odb_install_unknown
assertStringEq [some_missing_command] "app_unknown saw some_missing_command" \
  "odb_unknown did not chain to the displaced handler"
assertStringNotEq [$net getName] "" "handle dispatch broke after reinstall"
uplevel #0 [list namespace unknown ""]
odb_install_unknown
assert {[catch { no_such_command_at_all }] == 1} \
  "unknown command did not error after the handler was cleared"

# Values reach a constructed object intact.  Handing the pointer to Tcl encodes
# it byte-wise, which is not enough for gcc to treat the object as escaping, so
# without an explicit escape it drops the stores that initialize it.
set r [odb::new_Rect 100 200 300 400]
assert {[odb::Rect_xMin $r] == 100} "xMin is [odb::Rect_xMin $r], expected 100"
assert {[odb::Rect_yMin $r] == 200} "yMin is [odb::Rect_yMin $r], expected 200"
assert {[odb::Rect_xMax $r] == 300} "xMax is [odb::Rect_xMax $r], expected 300"
assert {[odb::Rect_yMax $r] == 400} "yMax is [odb::Rect_yMax $r], expected 400"
assert {[$r xMin] == 100} "xMin via dispatch is [$r xMin], expected 100"

set p [odb::new_Point 7 9]
assert {[$p getX] == 7} "getX is [$p getX], expected 7"
assert {[$p getY] == 9} "getY is [$p getY], expected 9"

puts "pass"
exit 0
