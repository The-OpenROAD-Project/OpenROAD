declare -a arr=("bsg_manycore_tile.sets" "bsg_rocket_node_client_rocc_sliced.sets" "ca53_cpu.sets")

for i in "${arr[@]}"
do
  echo "$i"
 ./Parquet.exe -f ./bench/other/${i} -plot -noRotation -s 100 -plotNoNets -minWL -savePl ${i}_out
 mv ${i}_out.pl ./bench/other_result/
 mv out.plt ${i}_minwl.plt
 gnuplot ${i}_minwl.plt > ${i}_minwl.png
 mv ${i}_minwl.plt ./bench/other_result/
 mv ${i}_minwl.png ./bench/other_result/
done

# mv out.plt ami33.plt

