lcov --capture --directory /OpenROAD/build --exclude '/usr/include/*' --exclude '/opt/*' --exclude '/usr/lib/*' --exclude '/usr/local/*' --exclude '*build*' --output-file /OpenROAD/main_coverage.info

genhtml /OpenROAD/main_coverage.info --output-directory /OpenROAD/out --ignore-errors source
