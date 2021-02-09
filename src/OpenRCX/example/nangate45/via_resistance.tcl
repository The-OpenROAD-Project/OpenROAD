###################################################################
set tech [ord::get_db_tech]

set layer [$tech findLayer via1]
$layer setResistance 5 

set layer [$tech findLayer via2]
$layer setResistance 5

set layer [$tech findLayer via3]
$layer setResistance 5

set layer [$tech findLayer via4]
$layer setResistance 3

set layer [$tech findLayer via5]
$layer setResistance 3

set layer [$tech findLayer via6]
$layer setResistance 3

set layer [$tech findLayer via7]
$layer setResistance 1

set layer [$tech findLayer via8]
$layer setResistance 1

set layer [$tech findLayer via9]
$layer setResistance 0.5

###################################################################
