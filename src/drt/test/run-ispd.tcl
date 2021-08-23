#!/usr/bin/tclsh

set run_dir "$::env(HOME)/ispd/runs"
set parallel_jobs 4


proc genFiles { run_dir ispd_year design drv } {
    # host setup
    set currentdir  [file normalize [file dirname [file normalize [info script]]]/../ ]
    set program     "$currentdir/../../build/src/openroad"
    set bench_dir   "$::env(HOME)/ispd/tests"

    # TritonRoute setup
    set verbose 1
    set threads [exec nproc]

    file mkdir $run_dir
    file mkdir $run_dir/$design

    puts "Create tcl script for $design"
    set    tclFile [open "$run_dir/$design/run.tcl" w]
    puts  $tclFile "set_thread_count $threads"
    puts  $tclFile "read_lef $bench_dir/$design/$design.input.lef"
    puts  $tclFile "read_def $bench_dir/$design/$design.input.def"
    puts  $tclFile "detailed_route -guide $bench_dir/$design/$design.input.guide \\"
    puts  $tclFile "               -output_guide $design.output.guide.mod \\"
    puts  $tclFile "               -output_maze $design.output.maze.log \\"
    puts  $tclFile "               -output_drc $design.output.drc.rpt \\"
    puts  $tclFile "               -verbose $verbose"
    puts  $tclFile "write_def $run_dir/$design/$design.output.def"
    puts  $tclFile "set drv_count \[detailed_route_num_drvs] "
    puts  $tclFile "if { \$drv_count > $drv } {"
    puts  $tclFile "  puts \"ERROR: Increase in number of violations from $drv to \$drv_count\""
    puts  $tclFile "  exit 1"
    puts  $tclFile "} elseif { \$drv_count < $drv } {"
    puts  $tclFile "  puts \"NOTICE: Decrease in number of violations from $drv to \$drv_count\""
    puts  $tclFile "  exit 2"
    puts  $tclFile "}"
    close $tclFile

    puts "Create run script for $design"
    set    runFile [open "$run_dir/$design/run.sh" w]
    puts  $runFile "set -e"
    puts  $runFile "cd $run_dir/$design"
    puts  $runFile "$program -exit run.tcl > $run_dir/$design/run.log"
    puts  $runFile "echo"
    puts  $runFile "echo"
    puts  $runFile "echo Report for $design"
    puts  $runFile "tail $run_dir/$design/run.log"
    puts  $runFile "cd '$bench_dir/ispd${ispd_year}eval'"
    puts  $runFile "./ispd${ispd_year}eval -lef $bench_dir/$design/$design.input.lef -def $run_dir/$design/$design.output.def -guide $bench_dir/$design/$design.input.guide | grep -v WARNING | grep -v ERROR"
    puts  $runFile "echo"
    close $runFile
    exec chmod +x "$run_dir/$design/run.sh"
}

set design_list_ispd18 " \
    ispd18_test10 \
    ispd18_test9 \
    ispd18_test8 \
    ispd18_test7 \
    ispd18_test6 \
    ispd18_test5 \
    ispd18_test4 \
    ispd18_test3 \
    ispd18_test2 \
    ispd18_test1 \
    "
set drvs_ispd18 { \
    0 \
    0 \
    0 \
    0 \
    0 \
    0 \
    8 \
    0 \
    0 \
    0 \
    }
set design_list_ispd19 " \
    ispd19_test10 \
    ispd19_test9 \
    ispd19_test8 \
    ispd19_test7 \
    ispd19_test6 \
    ispd19_test5 \
    ispd19_test4 \
    ispd19_test3 \
    ispd19_test2 \
    ispd19_test1 \
    "
set drvs_ispd19 { \
    27 \
    1 \
    0 \
    0 \
    0 \
    0 \
    0 \
    0 \
    0 \
    0 \
    }
foreach design $design_list_ispd18 drv $drvs_ispd18 {
    genFiles $run_dir 18 $design $drv
}

foreach design $design_list_ispd19 drv $drvs_ispd19 {
    genFiles $run_dir 19 $design $drv
}

cd $run_dir

set design_list [concat $design_list_ispd18 $design_list_ispd19]
set status [catch { eval exec -ignorestderr parallel -j $parallel_jobs --halt never --joblog $run_dir/log ./{}/run.sh ::: $design_list >@stdout } ]
foreach design $design_list {
    set fileName "$run_dir/$design/run.log"
    set f [open $fileName]
    fcopy $f stdout
    close $f
    exec tar czvf "${design}.tar.gz" "${design}"
}
puts "======================="
if $status {
    puts "Fail"
} else {
    puts "Success"    
}
puts "======================="
exit $status

