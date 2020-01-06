
################################################################################
## Authors: Vitor Bandeira, Eder Matheus Monteiro e Isadora Oliveira
##          (Advisor: Ricardo Reis)
##
## BSD 3-Clause License
##
## Copyright (c) 2019, Federal University of Rio Grande do Sul (UFRGS)
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and#or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
################################################################################

rename puts _puts

proc _err {s {n 1}} {
        _puts stderr "ERROR: $s"
        exit $n
}

proc _warn {s} {
        _puts "WARNING: $s"
}

proc _debug {s {n 1}} {
        global debugLevel
        if {$n <= $debugLevel} {
                _puts "DEBUG: $s"
        }
}

proc _info {s {n 0}} {
        global infoLevel
        if {$n <= $infoLevel} {
                _puts "INFO: $s"
        }
}

proc _infoN {s} {
        _puts -nonewline "INFO: $s"
}

proc runTapcell {testName testDir inputDir binFile outLog} {
        set lefFile "${inputDir}/${testName}.lef"
        set defFile "${inputDir}/${testName}.def"

        exec cp $testDir/insertTap.tcl $testDir/$testName.tcl
        exec sed -i s#_LEF_#$lefFile#g $testDir/$testName.tcl
        exec sed -i s#_DEF_#$defFile#g $testDir/$testName.tcl
        exec sed -i s#_GUIDE_#$testDir/$testName.guide#g $testDir/$testName.tcl
        catch {exec $binFile < $testDir/$testName.tcl > $outLog}
}

# proc Main {} {

set base_dir [pwd]
_puts $base_dir
set tests_dir "${base_dir}/src/tapcell/test"
set src_dir "${tests_dir}/src"
set inputs_dir "${tests_dir}/input"

_puts "Start unit tests..."

proc unit_tests {{dir}} {
        set subdirs [glob -dir $dir *]

        foreach subdir [split $subdirs] {
                source ${subdir}/run.tcl
        }
}

unit_tests $src_dir

# }
