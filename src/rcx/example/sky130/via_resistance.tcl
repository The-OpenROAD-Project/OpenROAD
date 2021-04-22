###################################################################
set tech [ord::get_db_tech]

#via 1
set layer [$tech findLayer mcon]
$layer setResistance 9.249146

#via 2
set layer [$tech findLayer via]
$layer setResistance 4.5 

#via 3
set layer [$tech findLayer via2]
$layer setResistance 3.368786

#via 4
set layer [$tech findLayer via3]
$layer setResistance 0.376635

#via 5
set layer [$tech findLayer via4]
$layer setResistance 0.00580

###################################################################
