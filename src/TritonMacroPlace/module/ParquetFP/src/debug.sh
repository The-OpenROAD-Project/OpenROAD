rm -rf out.plt
echo "Parquet.exe -verb 100_100_100 -f ca53_cpu.sets -minWL -noRotation -FPrep BTree -s 1 -outline 0,0,700,700 -plotNoNets -plotNoSlacks -savePl ca53 | tee ca53.log"
Parquet.exe -verb 100_100_100 -f ca53_cpu.sets -minWL -noRotation -FPrep BTree -s 70 -outline 0,0,700,700 -plotNoNets -plotNoSlacks -savePl ca53 | tee ca53.log
# gnuplot out.plt
