source "helpers.tcl"

cd ..
exec ../../etc/FindMessages.tcl > messages.txt

exec git diff --quiet messages.txt

