declare -a arr=("ami33" "ami49" "apte" "hp" "xerox")

for i in "${arr[@]}"
do
  echo "$i"
 ./Parquet.exe -f ./bench/mcnc/${i} -plot -noRotation -s 100 -plotNoNets -savePl ${i}_out 
 mv ${i}_out.pl ./bench/mcnc_result/
 mv out.plt ${i}_minwl.plt
 gnuplot ${i}_minwl.plt > ${i}_minwl.png
 mv ${i}_minwl.plt ./bench/mcnc_result/
 mv ${i}_minwl.png ./bench/mcnc_result/
done

# mv out.plt ami33.plt

