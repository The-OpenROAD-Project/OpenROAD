source "helpers.tcl"

set metrics_file [make_result_file "metrics.json"]

utl::open_metrics $metrics_file

utl::metric_integer "integer0" -1
utl::metric_integer "integer1" 0
utl::metric_integer "integer2" 1

utl::metric_float "float0" 0
utl::metric_float "float1" 1e3
utl::metric_float "float2" 1e39
# TCL cannot convert NaN
#utl::metric_float "float3" nan
utl::metric_float "float4" INF
utl::metric_float "float5" -INF

utl::metric "string0" ""
utl::metric "string1" "one"

utl::close_metrics $metrics_file
diff_files "metrics-tcl.jsonok" $metrics_file
