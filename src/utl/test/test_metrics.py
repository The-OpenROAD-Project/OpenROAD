import utl
import helpers

metrics_file = helpers.make_result_file("metrics.json")

utl.open_metrics(metrics_file)

utl.metric_integer("integer0", -1)
utl.metric_integer("integer1", 0)
utl.metric_integer("integer2", 1)

utl.metric_float("float0", 0)
utl.metric_float("float1", 1e3)
utl.metric_float("float2", 1e39)
utl.metric_float("float3", float("nan"))
utl.metric_float("float4", float("inf"))
utl.metric_float("float5", float("-inf"))

utl.metric("string0", "")
utl.metric("string1", "(one")

utl.close_metrics(metrics_file)
helpers.diff_files("metrics-py.jsonok", metrics_file)


utl.metric_float("float6", float("-inf"))
