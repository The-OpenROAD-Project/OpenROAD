#!/bin/bash

if [[ $# != 10 ]]; then
        echo "you need to give X arguments:"   
        echo "  1st - the ioPlacer binary path"
	echo "  2nd - ioPlacer arguments" 
        echo "  3rd - the RePlAce binary path"
	echo "  4th - RePlAce arguments"
        echo "  5th - the number of iterations"
        echo "  6th - a LEF file path"
        echo "  7th - a DEF file path"
        echo "  8th - output directory, where the logs and the final DEF will be saved"
        echo "	9th - DEF file name (without .def extension)"
	echo "	10th - Boolean to indicate if DEF has pin placement"
	exit 1
fi

ioplacerArgs="$2"
replaceArgs="$4"
iterations="$5"
ioBin="$1"
replaceBin="$3"
lefFile="$6"
defFile="$7"
outputdir="$8"
defName="$9"
hasIoPlace="${10}"

# Directory used to save the parcial DEFs of each iteration
runDir="runLoop"
replaceOutDir="${outputdir}/replace"
ioplaceOutDir="${outputdir}/ioplace"

mkdir -p $replaceOutDir
mkdir -p $ioplaceOutDir
mkdir -p $runDir

if [[ ! -f "${ioBin}" ]]; then
        echo "IOPlacement binary not found, you need to compile it before"
        exit 1
fi

if [[ ! -f "${replaceBin}" ]]; then
        echo "RePlAce binary not found, you need to compile it before"
        exit 1
fi

if [[ ! -f "${lefFile}" ]]; then
        echo "LEF file not found"
        exit 1
fi

if [[ ! -f "${defFile}" ]]; then
        echo "DEF file not found"
        exit 1
fi

cp ${defFile} ${runDir}/${defName}.def

if [[ "${hasIoPlace}" == 0  ]]
then
	"$ioBin" \
                -l ${lefFile} \
                -d ${defFile} \
                ${ioplacerArgs} \
                -o out.def

        mv out.def ${runDir}/${defName}.def
fi

# Main loop
replaceExperimentDir="${replaceOutDir}/etc/${defName}/experiment"
# do n iterations (user input)
for j in $(seq 0 "${iterations}")
do
        echo "----Iteration $j"
        logsFolder="${ioplaceOutDir}/it${j}/${defName}/logs"
        mkdir -p "$logsFolder"
        replaceExperimentDEF=${j}/${defName}_final.def

        echo "--------Running RePlAce"
        # Run RePlAce to generate a placed DEF
              
        #first iteration uses '-fast' flag for RePlAce and the input DEF
        if [[ "${j}" == 0 ]] 
        then
                echo "------------Creating initial def with '-fast' flag"
                if [[ "${hasIoPlace}" == 0  ]]
		then
			${replaceBin} \
				-bmflag etc \
				-lef ${lefFile} \
				-def ${runDir}/${defName}.def \
				${replaceArgs} \
				-output ${replaceOutDir} \
                        	-fast \
                        	> "$logsFolder"/replace_test"${i}"_it"${j}".log
		else
			${replaceBin} \
                                -bmflag etc \
                                -lef ${lefFile} \
                                -def ${defFile} \
                                ${replaceArgs} \
                                -output ${replaceOutDir} \
                                -fast \
                                > "$logsFolder"/replace_test"${i}"_it"${j}".log
		fi
        else 
                ${replaceBin} \
			-bmflag etc \
			-lef ${lefFile} \
			-def ${runDir}/${defName}.def \
			${replaceArgs} \
			-output ${replaceOutDir} \
                        > "$logsFolder"/replace_test"${i}"_it"${j}".log
        fi

        if [ "${j}" -lt 10 ]; then
                cp "${replaceExperimentDir}00${replaceExperimentDEF}" \
                        ${runDir}/${curr_test}.replace.def
        elif [ "${j}" -lt 100 ]; then
                cp "${replaceExperimentDir}0${replaceExperimentDEF}" \
                        ${runDir}/${curr_test}.replace.def
        else
                cp "${replaceExperimentDir}${replaceExperimentDEF}" \
                        ${runDir}/${curr_test}.replace.def
        fi

        echo "--------Running ioPlacer"
        # Run ioPlacer to place the IO pins for the placed DEF
        ioLogFile="${logsFolder}/ioPlacer_test${i}_it${j}.log"
              
        "$ioBin" \
                -l ${lefFile} \
                -d ${runDir}/${curr_test}.replace.def \
                ${ioplacerArgs} \
		-o out.def > "$ioLogFile"
        
	mv out.def ${runDir}/${defName}.def
done

finalDEFs="finalDEFs"
mkdir -p ${outputdir}/${finalDEFs}
mv ${runDir}/${defName}.def ${outputdir}/${finalDEFs}/${defName}.final.def
rm -Rf ${runDir}
