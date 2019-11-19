docker run -v $(pwd):/OpenROAD openroad/openroad bash -c "/OpenROAD/test/regression fast && /OpenROAD/src/resizer/test/regression fast"
