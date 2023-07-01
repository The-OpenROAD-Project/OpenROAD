#!/bin/sh

# First step is to call find_messages.py to generate the messages.txt file
# Next is to parse it to get the error <NUM> -> create <NUM>.md
# Logic: if file exists in tool/doc/messages, do not create a new one. 
# Otherwise, create for the user using the basic template. 
#
# <NUM>.md should just contain 
# -----------------------------
# Description of how to handle the error, or more information that may be helpful.
#
# Changelog:
# ==========
# 010723: For now, we are just trying prototype so limited to first two tool (ant)
cwd=$(pwd)
for i in $(ls -d ../src/*/); do
 #rm -rf  "$i"doc/messages
 if [[ ! $i =~ "ant" ]] && [[ ! $i =~ "cts" ]]; then
    continue
 fi
 cd $cwd && echo $i
 path=$(readlink -f ${i})
 echo $path && cd $path
 python ../../etc/find_messages.py > messages.txt
 file_nums=$(awk '{print $2}' messages.txt)
 mkdir -p doc/messages && cd doc/messages
 for num in $file_nums; do
   if [ ! -f "$num".md ]; then
     touch "$num".md 
   else
     echo "Skipping $num.md"
   fi
 done
done

