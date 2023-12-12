# man wrapper function
proc man args {
    exec {*}[auto_execok man] {*}$args <@stdin >@stdout 2>@stderr
}

proc clear {} {
    exec clear
}