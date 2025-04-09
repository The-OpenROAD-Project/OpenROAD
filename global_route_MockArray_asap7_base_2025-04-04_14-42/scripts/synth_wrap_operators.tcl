set deferred_cells {
  {
    \$alu
    ALU_{A_WIDTH}_{A_SIGNED}_{B_WIDTH}_{B_SIGNED}_{Y_WIDTH}{%unused}
    {HAN_CARLSON -map +/choices/han-carlson.v}
    {KOGGE_STONE -map +/choices/kogge-stone.v}
    {SKLANSKY -map +/choices/sklansky.v}
    {BRENT_KUNG}
  }
  {
    \$macc
    MACC_{CONFIG}_{Y_WIDTH}{%unused}
    {BASE -map +/choices/han-carlson.v}
    {BOOTH -max_iter 1 -map ../flow/scripts/synth_wrap_operators-booth.v}
  }
}

techmap {*}[join [lmap cell $deferred_cells {string cat "-dont_map [lindex $cell 0]"}] " "]

foreach info $deferred_cells {
  set type [lindex $info 0]
  set naming_template [lindex $info 1]
  # default architecture and its suffix
  set default [lindex $info 2]
  set default_suffix [lindex $default 0]

  log -header "Generating architectural options for $type"
  log -push

  wrapcell \
    -setattr arithmetic_operator \
    -setattr copy_pending \
    -formatattr implements_operator $naming_template \
    -formatattr architecture $default_suffix \
    -formatattr source_cell $type \
    -name ${naming_template}_${default_suffix} \
    t:$type r:A_WIDTH>=10 r:Y_WIDTH>=14 %i %i

  # make per-architecture copies of the unmapped module
  foreach modname [tee -q -s result.string select -list-mod A:arithmetic_operator A:copy_pending %i] {
    setattr -mod -unset copy_pending $modname

    # iterate over non-default architectures
    foreach arch [lrange $info 3 end] {
      set suffix [lindex $arch 0]
      set base [rtlil::get_attr -string -mod $modname implements_operator]
      set newname ${base}_${suffix}
      yosys copy $modname $newname
      yosys setattr -mod -set architecture \"$suffix\" $newname
    }
  }

  # iterate over all architectures, both the default and non-default
  foreach arch [lrange $info 2 end] {
    set suffix [lindex $arch 0]
    set extra_map_args [lrange $arch 1 end] 

    # map all operator copies which were selected to have this architecture
    techmap -map +/techmap.v {*}$extra_map_args A:source_cell=$type A:architecture=$suffix %i

    # booth isn't able to map all $macc configurations: catch if this is one
    # of those and delete the option
    delete A:source_cell=$type A:architecture=$suffix %i t:\$macc %m %i
  }

  log -pop
}

opt -fast -full
memory_map
opt -full
# Get rid of indigestibles
chformal -remove
setattr -mod -set abc9_script {"+&dch;&nf -R 5;"} A:arithmetic_operator
setattr -mod -set abc9_box 1 A:arithmetic_operator
techmap -map +/techmap.v -map +/choices/han-carlson.v
