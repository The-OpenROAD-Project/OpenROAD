#!/usr/bin/tclsh

set run_dir "$::env(HOME)/ispd/runs"
set parallel_jobs 4


proc genFiles { run_dir ispd_year design } {
    # host setup
    set currentdir  [file normalize [file dirname [file normalize [info script]]]/../ ]
    set program     "$currentdir/../../build/src/openroad"
    set bench_dir   "$::env(HOME)/ispd/tests"

    # TritonRoute setup
    set verbose 1
    set threads [exec nproc]

    file mkdir $run_dir
    file mkdir $run_dir/$design
    puts "Create param file for $design"
    set    paramFile [open "$run_dir/$design/run.param" w]
    puts  $paramFile "guide:$bench_dir/$design/$design.input.guide"
    puts  $paramFile "outputguide:$design.output.guide.mod"
    puts  $paramFile "outputMaze:$design.output.maze.log"
    puts  $paramFile "outputDRC:$design.outputDRC.rpt"
    puts  $paramFile "threads:$threads"
    puts  $paramFile "verbose:$verbose"
    close $paramFile

    puts "Create tcl script for $design"
    set    tclFile [open "$run_dir/$design/run.tcl" w]
    puts  $tclFile "read_lef $bench_dir/$design/$design.input.lef"
    puts  $tclFile "read_def $bench_dir/$design/$design.input.def"
    puts  $tclFile "detailed_route -param $run_dir/$design/run.param"
    puts  $tclFile "write_def $run_dir/$design/$design.output.def"
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

foreach design $design_list_ispd18 {
    genFiles $run_dir 18 $design
}

foreach design $design_list_ispd19 {
    genFiles $run_dir 19 $design
}

cd $run_dir

set design_list [concat $design_list_ispd18 $design_list_ispd19]
set status [catch { eval exec -ignorestderr parallel -j $parallel_jobs --halt never --joblog $run_dir/log ./{}/run.sh ::: $design_list >@stdout } ]
foreach design $design_list {
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

