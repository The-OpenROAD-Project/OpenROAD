docker run -v $(pwd):/OpenROAD openroad/openroad bash -c "/OpenROAD/test/regression fast && /OpenROAD/src/resizer/test/regression fast"
docker run -v $(pwd):/OpenROAD openroad/openroad bash -c "cd /OpenROAD/src/replace/test && python3 regression.py run openroad"
