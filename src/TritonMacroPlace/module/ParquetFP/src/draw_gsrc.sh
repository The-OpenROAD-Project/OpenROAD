declare -a arr=("n10" "n30" "n50" "n100" "n200" "n300")

for i in "${arr[@]}"
do
  echo "$i"
 ./Parquet.exe -f ./bench/gsrc/${i} -plot -noRotation -s 100 -plotNoNets -minWL -savePl ${i}_out 
 mv ${i}_out.pl ./bench/gsrc_result/
 mv out.plt ${i}_minwl.plt

 gnuplot ${i}_minwl.plt > ${i}_minwl.png
 mv ${i}_minwl.plt ./bench/gsrc_result/
 mv ${i}_minwl.png ./bench/gsrc_result/
done

# mv out.plt ami33.plt

